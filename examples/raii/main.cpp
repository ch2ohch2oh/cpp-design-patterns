// Pattern: RAII (Resource Acquisition Is Initialization)
//
// Backend relevance:
// - Sockets, files, mutexes, and other resources must be released reliably.
// - RAII ties resource lifetime to scope, so cleanup happens automatically on
//   all paths (success, early-return, exceptions).
//
// This example wraps a C `FILE*` in a small RAII type and writes a tiny log.
//
// Notes on constructors + exceptions (best practice):
// - Throwing from a constructor is fine when you cannot create a valid object.
// - If a constructor throws, the object's destructor does NOT run (because the object
//   never finished constructing).
// - However, fully-constructed members/subobjects ARE destroyed during stack unwinding.
//   So put resources into RAII members (like `std::unique_ptr` with a deleter), and
//   you won't leak even if the constructor throws later.
// - If you need a non-throwing API, prefer a factory returning `std::optional` /
//   `std::expected` (or a separate `init()`), instead of a partially-initialized object.

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

class FileHandle {
   public:
    explicit FileHandle(const char* path, const char* mode) : file_(nullptr, &std::fclose) {
        std::FILE* f = std::fopen(path, mode);
        const int err = errno;
        if (!f) {
            throw std::runtime_error(std::string("fopen failed: ") + std::strerror(err));
        }
        file_.reset(f);
    }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // Movable by default: allows transferring ownership safely.
    FileHandle(FileHandle&&) noexcept = default;
    FileHandle& operator=(FileHandle&&) noexcept = default;

    // Escape hatch for C APIs.
    //
    // Safety rules (callers must follow):
    // - This pointer is non-owning: do NOT call `fclose()` on it.
    // - Do NOT store it past this `FileHandle`'s lifetime.
    // - Do NOT use it after moving from this object.
    //
    // In production code, you can often avoid exposing the raw handle by
    // providing wrapper methods (e.g. `write_line()`, `flush()`).
    std::FILE* get() const { return file_.get(); }

   private:
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file_;
};

static void write_lines(std::FILE* file, std::string_view prefix, int count) {
    for (int i = 0; i < count; ++i) {
        std::string line = std::string(prefix) + " line " + std::to_string(i) + "\n";
        if (std::fputs(line.c_str(), file) == EOF) {
            throw std::runtime_error("write failed");
        }
    }
}

int main() {
    try {
        std::cout << "RAII example: writing a small log file.\n";

        FileHandle log("raii.log", "w");
        write_lines(log.get(), "request", 3);

        std::cout << "Wrote raii.log; handle closes automatically.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
