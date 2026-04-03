#pragma once

#include <iostream>
#include <mutex>
#include <utility>

namespace examples {

inline std::mutex& log_mutex() {
    static std::mutex mu;
    return mu;
}

template <class... Ts>
void log_line(Ts&&... parts) {
    std::lock_guard<std::mutex> lock(log_mutex());
    ((std::cout << std::forward<Ts>(parts)), ...) << "\n";
}

}  // namespace examples
