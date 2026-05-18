#include "main.hpp"

#include "arch/x86_64/syscall/syscall.h"
#include "std/math_types.hpp"
#include "std/types.hpp"

namespace Chess {
    constexpr float rect_size = 0.1f;

    void main() {
        while (true) {
            //auto key = sys_get_key();
            //if (key == kb::key_code::KEY_ESC) {
            //    return;
            //}

            // Render board
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    auto start = glm::vec2(0.1f+(rect_size*x), 0.1f+(rect_size*y));
                    auto size = glm::vec2(rect_size);
                    int color = 0xEEEED2;
                    if ((x+y) % 2 == 0)
                        color = 0x769652;
                    sys_draw_rectangle(&start, &size, color);
                }
            }





            sys_swap_framebuffer();
        }
    }
}