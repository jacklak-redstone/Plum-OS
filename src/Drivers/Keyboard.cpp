#include "Keyboard.hpp"
#include <arch/x86_64/Common/Common.hpp>
#include "std/types.hpp"
#include "kernel/system.hpp"

namespace kb {
    mem::Ring_Buffer<uint8_t, 256> buf = {};

    /*
    Waits for char in buffer
     */
    key_code get_char() {
        while (buf.empty()) {
            systemPL::fb.swap();
            asm volatile("sti; hlt; cli");
        }

        uint8_t sc;
        buf.pop(sc);
        return static_cast<key_code>(sc);
    }

    /*
    Reads char from buffer
    If there's no char (or char cant be mapped to map) it outputs 0
     */
    key_code read_char() {
        if (buf.empty())
            return key_code::KEY_NULL;

        uint8_t sc;
        buf.pop(sc);
        return static_cast<key_code>(sc);
    }

    char to_char(key_code code) {
        return ascii_map[static_cast<uint8_t>(code)];
    }
}
