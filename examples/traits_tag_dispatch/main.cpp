// Pattern: Traits / tag dispatch / (concepts-style selection)
//
// Backend relevance:
// - Backend code often needs one API that accepts many types (IDs, strings, small structs)
//   for hashing/serialization/metrics keys without paying runtime polymorphism costs.
// - A small trait + tag dispatch keeps call sites clean while still selecting fast paths.
//
// This example builds a cache key from heterogeneous parts:
// - Trivially-copyable types -> append raw bytes (fast path; note endianness caveat)
// - String-like types -> append characters + delimiter
// - Custom structs -> provide a trait specialization and an overload for their encoding
//
// What is a "trait" here?
// - `KeyPartTag<T>` is the trait: a compile-time mapping from a type `T` to a "tag type".
// - Specializing `KeyPartTag<T>` changes behavior without changing call sites.
// - The selected tag is then used for tag dispatch in `append_part(out, value, Tag{})`.
//
// Another common production use of traits is logging/redaction:
// - `LogTraits<T>` maps a type `T` to a safe-to-log representation.
// - The default can be redacted, with explicit opt-in specializations for safe types.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../common/log.h"

namespace examples::traits_example {

using Bytes = std::vector<std::uint8_t>;

inline void append_bytes(Bytes& out, const void* data, std::size_t size) {
    const auto* begin = static_cast<const std::uint8_t*>(data);
    out.insert(out.end(), begin, begin + size);
}

inline void append_string(Bytes& out, std::string_view s) {
    append_bytes(out, s.data(), s.size());
    out.push_back(0);  // delimiter so boundaries are unambiguous
}

struct BytesTag {};
struct StringTag {};
struct UserKeyTag {};

template <class T>
struct KeyPartTag {
    using type = BytesTag;
};

template <>
struct KeyPartTag<std::string> {
    using type = StringTag;
};

template <>
struct KeyPartTag<std::string_view> {
    using type = StringTag;
};

template <class T>
using KeyPartTagT = typename KeyPartTag<std::remove_cvref_t<T>>::type;

template <class T>
void append_part(Bytes& out, const T& value, BytesTag) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Default BytesTag path requires trivially copyable types.");
    // For demo simplicity we append native bytes. For stable keys across machines,
    // normalize endianness and define a stable encoding.
    append_bytes(out, std::addressof(value), sizeof(T));
}

inline void append_part(Bytes& out, std::string_view value, StringTag) {
    append_string(out, value);
}

struct UserKey {
    std::uint64_t user_id;
    std::string region;
};

template <>
struct KeyPartTag<UserKey> {
    using type = UserKeyTag;
};

inline void append_part(Bytes& out, const UserKey& key, UserKeyTag) {
    out.push_back(0xA5);  // simple "type tag" for demo
    append_part(out, key.user_id, BytesTag{});
    append_part(out, std::string_view{key.region}, StringTag{});
}

template <class T>
void append_part(Bytes& out, const T& value) {
    append_part(out, value, KeyPartTagT<T>{});
}

std::string to_hex(const Bytes& bytes) {
    static constexpr std::array<char, 16> kHex = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };

    std::string out;
    out.reserve(bytes.size() * 2);
    for (std::uint8_t b : bytes) {
        out.push_back(kHex[(b >> 4) & 0xF]);
        out.push_back(kHex[b & 0xF]);
    }
    return out;
}

template <class T>
struct LogTraits {
    static std::string to_string(const T&) { return "[redacted]"; }
};

template <>
struct LogTraits<std::uint32_t> {
    static std::string to_string(std::uint32_t v) { return std::to_string(v); }
};

template <>
struct LogTraits<std::uint64_t> {
    static std::string to_string(std::uint64_t v) { return std::to_string(v); }
};

template <>
struct LogTraits<std::string_view> {
    static std::string to_string(std::string_view v) { return std::string{v}; }
};

struct Email {
    std::string value;
};

template <>
struct LogTraits<Email> {
    static std::string to_string(const Email&) { return "[redacted-email]"; }
};

template <>
struct LogTraits<UserKey> {
    static std::string to_string(const UserKey& key) {
        return "UserKey{user_id=" + std::to_string(key.user_id) + ", region=" + key.region + "}";
    }
};

template <class T>
std::string log_value(const T& v) {
    return LogTraits<std::remove_cvref_t<T>>::to_string(v);
}

}  // namespace examples::traits_example

int main() {
    using namespace examples::traits_example;

    examples::log_line("Traits/tag dispatch example: build a cache key efficiently.");

    Bytes key;
    key.reserve(128);

    const std::string_view route = "/users/get";
    const std::uint32_t request_id = 42;
    const UserKey user_key{.user_id = 9001, .region = "us-east-1"};
    const Email email{.value = "alice@example.com"};

    append_part(key, route);
    append_part(key, request_id);
    append_part(key, user_key);

    examples::log_line("cache key bytes (hex): ", to_hex(key));
    examples::log_line("safe log values: route=", log_value(route),
                       " request_id=", log_value(request_id), " user_key=", log_value(user_key),
                       " email=", log_value(email));
}
