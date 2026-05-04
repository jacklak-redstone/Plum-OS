#pragma once
#include "libs/std/types.hpp"

namespace PIT {
    extern uint32_t ticks_per_ms;

    void set_PIT_timer_freq(uint32_t hz);

    void set_aPIC_timer_freq(uint32_t hz);

    void calibrate_aPIC_timer();
}