#pragma once

#include <chrono>
#include "types.h"

namespace Time {

    using Nanoseconds = std::chrono::duration<i64, std::nano>;
    using Milliseconds = std::chrono::duration<i64, std::milli>;
    using Seconds = std::chrono::duration<i64>;

    using Clock = std::chrono::steady_clock;
    using Stamp = Clock::time_point;
    using Duration = Nanoseconds;

}

struct Timer {
    static constexpr Time::Duration DEFAULT_INTERVAL = Time::Seconds(1);

    Time::Duration interval = DEFAULT_INTERVAL;
    Time::Duration accumulator = Time::Duration::zero();
    u64 cycles = 0;
};

[[nodiscard]] bool timer_ready(Timer* timer) {
    return timer->accumulator >= timer->interval;
}

[[nodiscard]] Time::Duration timer_remaining(Timer* timer) {
    return timer->interval - timer->accumulator;
}

void timer_accumulate(Timer* timer, Time::Duration delta) {
    timer->accumulator += delta;
}

void timer_reset(Timer* timer) {
    timer->accumulator = Time::Duration::zero();
}

void timer_consume(Timer* timer) {
    if(timer_ready(timer)) {
        timer->accumulator -= timer->interval;
        ++timer->cycles;
    }
}