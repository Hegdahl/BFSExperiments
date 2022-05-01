#include <chrono>
#include <string>

#define TIME(...)                                                             \
  do {                                                                        \
    auto _start_time = std::chrono::steady_clock::now();                      \
    __VA_ARGS__;                                                              \
    auto _end_time = std::chrono::steady_clock::now();                        \
    auto _milliseconds =                                                      \
        std::chrono::duration_cast<std::chrono::milliseconds>(_end_time -     \
                                                              _start_time)    \
            .count();                                                         \
    std::cout << #__VA_ARGS__ << ": " << _milliseconds << " ms" << std::endl; \
  } while (0)
