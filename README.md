# sleep_for
Alternative implementation for Microsoft C++ runtime std::this_thread::sleep_for which is buggy as it uses the system clock which isn't steady.

See https://developercommunity.visualstudio.com/content/problem/58530/bogus-stdthis-threadsleep-for-implementation.html

It would even be more beneficial to implement the sleep_for/wait_for functions without the detour via absolute time and directly use Wondows' Sleep function. In an ideal world we could choose the chrono clock for sleeping/waiting which is a long missing feature at least under Windows where it a sub-millisecond sleep/wait is still not available.
Like i.e. sleeping/waiting on std::chrono::steady_clock which is based on QuerePerformanceFrequency/QueryPerformanceCounter.

The implementation in this repo is far from being perfect. It's just meant to be a basis for discussion.
