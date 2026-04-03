// Pattern: Rule of 0 / Rule of 5
//
// Backend relevance:
// - Prefer the Rule of 0: compose with standard library types so you don't write
//   custom destructors/copy/move operations (fewer bugs, better exception safety).
// - When you must own a raw resource (FFI, custom allocators, OS handles), follow
//   the Rule of 5: define (or delete) destructor, copy/move ctor, copy/move assign.
//
// This file shows both:
// - `ResponseBuffer` (Rule of 0): no special members needed.
// - `Arena` (Rule of 5): manual malloc/free, so copy/move behavior is explicit.

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Rule of 0: no raw owning pointers, no custom destructor.
// The compiler-generated special members are correct because `std::vector` and
// `std::string` already manage their own resources.
class ResponseBuffer {
   public:
    void append(std::string chunk) { chunks_.push_back(std::move(chunk)); }

    std::string join() const {
        std::string out;
        std::size_t total = 0;
        for (const auto& c : chunks_) total += c.size();
        out.reserve(total);
        for (const auto& c : chunks_) out += c;
        return out;
    }

   private:
    std::vector<std::string> chunks_;
};

// Rule of 5: manual resource ownership means you must define correct copy/move semantics.
// This simulates a small arena that might be used for C library interop.
class Arena {
   public:
    explicit Arena(std::size_t bytes)
        : size_(bytes), data_(bytes == 0 ? nullptr : static_cast<char*>(std::malloc(bytes))) {
        if (bytes != 0 && !data_) throw std::bad_alloc();
    }

    // (1) Destructor: releases the owned resource.
    ~Arena() { std::free(data_); }

    // (2) Copy constructor: deep-copy to avoid double-free.
    Arena(const Arena& other)
        : size_(other.size_),
          data_(other.size_ == 0 ? nullptr : static_cast<char*>(std::malloc(other.size_))) {
        if (other.size_ != 0 && !data_) throw std::bad_alloc();
        if (size_ != 0) std::memcpy(data_, other.data_, size_);
    }

    // (3) Copy assignment: use copy-and-swap for strong exception safety.
    Arena& operator=(const Arena& other) {
        if (this == &other) return *this;
        Arena tmp(other);
        swap(tmp);
        return *this;
    }

    // (4) Move constructor: transfer ownership; leave source empty.
    Arena(Arena&& other) noexcept : size_(other.size_), data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    // (5) Move assignment: release current resource, then take ownership.
    Arena& operator=(Arena&& other) noexcept {
        if (this == &other) return *this;
        std::free(data_);
        size_ = other.size_;
        data_ = other.data_;
        other.size_ = 0;
        other.data_ = nullptr;
        return *this;
    }

    void write(std::size_t offset, std::string_view s) {
        if (offset + s.size() > size_) throw std::out_of_range("arena write");
        std::memcpy(data_ + offset, s.data(), s.size());
    }

    std::string read(std::size_t offset, std::size_t len) const {
        if (offset + len > size_) throw std::out_of_range("arena read");
        return std::string(data_ + offset, data_ + offset + len);
    }

    void swap(Arena& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

   private:
    std::size_t size_{0};
    char* data_{nullptr};
};

int main() {
    ResponseBuffer buf;
    buf.append("hello ");
    buf.append("backend ");
    buf.append("world");
    std::cout << "Rule of 0: " << buf.join() << "\n";

    Arena a1(64);
    a1.write(0, "cache-key");
    Arena a2 = a1;             // copy: deep copy
    Arena a3 = std::move(a1);  // move: transfer ownership

    std::cout << "Rule of 5 (copy): " << a2.read(0, 9) << "\n";
    std::cout << "Rule of 5 (move): " << a3.read(0, 9) << "\n";
}
