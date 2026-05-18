#include "Render.hpp"
#include "std/math_types.hpp"
#include "std/types.hpp"
#include "kernel/system.hpp"

namespace renderer {
    void rectangle(const glm::vec2 *pos1, const glm::vec2 *size, const uint32_t color) {
        const auto fb = &systemPL::fb;
        auto info = fb->get_screen_info();

        auto start = screen_pos(*pos1);
        auto s = screen_size(*size);

        if (start.x < 0 || start.y < 0 || start.x >= info.width || start.y >= info.height)
            return;

        for (int y = start.y; y < start.y+s.y; y++) {
            for (int x = start.x; x < start.x+s.x; x++) {
                fb->set_pixel(x, y, color);
            }
        }

        fb->set_dirty();
    }

    glm::ivec2 screen_size(const glm::vec2 vec) {
        const auto fb = &systemPL::fb;
        auto info = fb->get_screen_info();
        return {
            static_cast<int>(vec.x * info.width),
            static_cast<int>(vec.y * info.height)
        };
    }

    glm::ivec2 screen_pos(const glm::vec2 vec) {
        const auto fb = &systemPL::fb;
        auto info = fb->get_screen_info();
        return {
            static_cast<int>(vec.x * (info.width -1)),
            static_cast<int>(vec.y * (info.height -1))
        };
    }
}