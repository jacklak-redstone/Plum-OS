#pragma once
#include "std/math_types.hpp"
#include "std/types.hpp"

namespace renderer {
    void rectangle(const glm::vec2 *pos1, const glm::vec2 *size, uint32_t color);

    glm::ivec2 screen_pos(glm::vec2 vec);
    glm::ivec2 screen_size(glm::vec2 vec);
}
