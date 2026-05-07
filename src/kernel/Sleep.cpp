#include "Sleep.hpp"
#include <arch/x86_64/Common/Common.hpp>

#include "log.h"
#include "arch/x86_64/IDT/PIT.hpp"
#include "Drivers/Keyboard.hpp"
#include "Drivers/hpet/hpet.h"

namespace Time {
    uint64_t hz;
    volatile uint64_t tick = 0;

    /*
     1000 -> 1s, 500 -> 0.5s
     10ms accuracy  (means 4ms is technically still 0ms)
    */
    void Sleep(const uint64_t t) {
        hpet::sleep_ms(t);
        return;

        const uint64_t end = tick + ((t * hz) / 1000);

        while (tick < end) {
            x64::halt();
        }
    }

    /*
     1000 -> 1s, 500 -> 0.5s
     true -> Cancel
     false -> time passed
    */
    bool WaitForKey(const uint64_t t, const kb::key_code key) {
        const uint64_t end = tick + ((t * hz) / 1000);

        while (tick < end) {
            if (kb::read_char() == key)
                return true;
            x64::halt();
        }
        return false;
    }

    /*
    Set Hardware clock frequency: (in hz)
    10hz <-> 10khz  (100hz recomended)
    */
    void Set_PIT(const uint64_t freq) {
        log::info("Setting PIT Frequency to %l...", freq);
        PIT::set_aPIC_timer_freq(freq);
        hz = freq;
    }
}
