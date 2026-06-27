#include "Sleep.hpp"
#include <arch/x86_64/Common/Common.hpp>

#include "log.h"
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
}
