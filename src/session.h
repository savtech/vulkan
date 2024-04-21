#pragma once

#include "types.h"
#include "time.h"

struct FPS {
    static constexpr Time::Duration MEASUREMENT_INTERVAL = Time::Milliseconds(300);

    Time::Stamp measurement_start_time = Time::Clock::now();
    u32 frames = 0;
    f64 last_measurement = 0.0;
    Timer timer = { .interval = MEASUREMENT_INTERVAL };
};

struct Session {
    u64 frames = 0;
    u64 ticks = 0;
    Time::Stamp start = Time::Clock::now();
    FPS fps = {};
    bool display_fps = false;
};

[[nodiscard]] Time::Duration session_running_time(Session* session) {
    return Time::Clock::now() - session->start;
}

void session_update(Session* session, Time::Duration delta_time) {
    ++session->ticks;
    timer_accumulate(&session->fps.timer, delta_time);

    new FPS();

    if(timer_ready(&session->fps.timer)) {
        Time::Stamp now = Time::Clock::now();
        f64 measurement_delta = std::chrono::duration_cast<std::chrono::duration<f64, std::chrono::seconds::period>>(now - session->fps.measurement_start_time).count();
        session->fps.last_measurement = static_cast<f64>((session->fps.frames / measurement_delta));
        session->fps.measurement_start_time = now;
        session->fps.frames = 0;
        timer_consume(&session->fps.timer);
    }
}

void session_render(Session* session) {
    ++session->frames;
    ++session->fps.frames;
}

void session_debug_print(Session* session) {
    printf("\nSession Info:\n");

    f64 elapsed_time_seconds = static_cast<f64>(session_running_time(session).count() / 1'000'000'000.0); //1 BILLION nanoseconds in a second
    u32 hours = static_cast<u32>(elapsed_time_seconds / 3600);
    u32 minutes = static_cast<u32>((elapsed_time_seconds - (hours * 3600)) / 60);
    f64 seconds = elapsed_time_seconds - (hours * 3600) - (minutes * 60);

    printf("Elapsed Time: %02d:%02d:%05.2f\n", hours, minutes, seconds);
    printf("Total Frames: %zd\n", session->frames);
    printf("Average FPS: %00007.2f", static_cast<f64>(session->frames / elapsed_time_seconds));
}