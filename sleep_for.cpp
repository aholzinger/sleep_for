// sleep_for.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>

#ifdef max
#undef max
#endif

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#define TIME_STEADY 2

constexpr long long Epoch = 0x19DB1DED53E8000LL;
constexpr long Nsec_per_sec = 1000000000L;
constexpr long Nsec100_per_sec = Nsec_per_sec / 100;

void GetPerformanceCountPreciseAsFileTime(LPFILETIME pft) { // get performance counter time in 100-nanosecond intervals since the epoch
    const long long freq = _Query_perf_frequency(); // doesn't change after system boot
    const long long ctr = _Query_perf_counter();
    // Instead of just having "ctr / freq",
    // the algorithm below prevents overflow when ctr is sufficiently large.
    // It is not realistic for ctr to accumulate to large values from zero with this assumption,
    // but the initial value of ctr could be large.
    const long long whole = ctr / freq;
    const long long part = ctr % freq;
    const long long result = whole * Nsec100_per_sec + (part * Nsec100_per_sec) / freq + Epoch;
    pft->dwHighDateTime = static_cast<DWORD>(result >> 32);
    pft->dwLowDateTime = static_cast<DWORD>(result & std::numeric_limits<DWORD>::max());
}

long long _Xtime_get_ticks_steady() { // get system time in 100-nanosecond intervals since the epoch
    FILETIME ft;
    GetPerformanceCountPreciseAsFileTime(&ft);
    long long result = ((static_cast<long long>(ft.dwHighDateTime)) << 32) + static_cast<long long>(ft.dwLowDateTime);
    result -= Epoch;
    return result;
}

static void sys_get_time(xtime* xt) { // get system time with nanosecond resolution
    unsigned long long now = _Xtime_get_ticks();
    xt->sec = static_cast<__time64_t>(now / Nsec100_per_sec);
    xt->nsec = static_cast<long>(now % Nsec100_per_sec) * 100;
}

static void steady_get_time(xtime* xt) { // get system time with nanosecond resolution
    unsigned long long now = _Xtime_get_ticks_steady();
    xt->sec = static_cast<__time64_t>(now / Nsec100_per_sec);
    xt->nsec = static_cast<long>(now % Nsec100_per_sec) * 100;
}

int xtime_get(xtime* xt, int type) { // get current time
    if (type != TIME_UTC && type != TIME_STEADY || xt == nullptr) {
        type = 0;
    }
    else if (type == TIME_STEADY) {
        steady_get_time(xt);
    }
    else {
        sys_get_time(xt);
    }

    return type;
}

void thrd_sleep(const xtime* xt, int type) { // suspend thread until time xt
    xtime now;
    xtime_get(&now, type);
    do { // sleep and check time
        long diff = _Xtime_diff_to_millis2(xt, &now);
        Sleep(diff);
        xtime_get(&now, type);
    } while (now.sec < xt->sec || now.sec == xt->sec && now.nsec < xt->nsec);
}

template <class Rep, class Period, class Clock = std::chrono::system_clock>
_NODISCARD bool _To_xtime_10_day_clamped(_CSTD xtime& xt, const std::chrono::duration<Rep, Period>& rel_time) noexcept(std::is_arithmetic<Rep>) {
    // Convert duration to xtime, maximum 10 days from now, returns whether clamping occurred.
    // If clamped, timeouts will be transformed into spurious non-timeout wakes, due to ABI restrictions where
    // the other side of the DLL boundary overflows int32_t milliseconds.
    // Every function calling this one is TRANSITION, ABI
    constexpr std::chrono::nanoseconds ten_days{ std::chrono::hours{24} *10 };
    constexpr std::chrono::duration<double> ten_days_d{ ten_days };
    std::chrono::nanoseconds tx0 = Clock::now().time_since_epoch();
    const bool clamped = ten_days_d < rel_time;
    if (clamped) {
        tx0 += ten_days;
    }
    else {
        tx0 += std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time);
    }

    const auto whole_seconds = std::chrono::duration_cast<std::chrono::seconds>(tx0);
    xt.sec = whole_seconds.count();
    tx0 -= whole_seconds;
    xt.nsec = static_cast<long>(tx0.count());
    return clamped;
}

template <class Clock, class Duration>
void sleep_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
    for (;;) {
        const auto now = Clock::now();
        if (abs_time <= now) {
            return;
        }

        _CSTD xtime tgt;
        (void)_To_xtime_10_day_clamped<std::chrono::time_point<Clock, Duration>::rep, std::chrono::time_point<Clock, Duration>::period, Clock>(tgt, abs_time - now);
        thrd_sleep(&tgt, Clock::is_steady ? TIME_STEADY : TIME_UTC);
    }
}

template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& rel_time) {
    const std::chrono::time_point<std::chrono::steady_clock> abs_time = std::chrono::steady_clock::now() + rel_time;
    sleep_until(abs_time);
}

int main()
{
    std::cout << "Sleeping for 10ms...";
    sleep_for(std::chrono::milliseconds(10));
    std::cout << " done.\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
