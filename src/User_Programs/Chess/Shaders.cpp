#include "main.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "std/math.hpp"
#include "std/trigonometry.hpp"

namespace Chess {
    using namespace OpenPL;

    void vshader(const Shader::VS_ShaderIn *In, Shader::VS_ShaderOut *out, void *uniform) {
        const auto uni = static_cast<uniforms *>(uniform);
        const glm::vec3 pos = *reinterpret_cast<glm::vec3 *>(In->attributes[0].data) + uni->offset;
        const glm::vec3 color = *reinterpret_cast<glm::vec3 *>(In->attributes[1].data);
        const float pitch = uni->pitch;
        const float yaw = uni->yaw;

        const float sx = std::sin(yaw);
        const float cx = std::cos(yaw);

        const float sy = std::sin(pitch);
        const float cy = std::cos(pitch);

        glm::vec3 p = pos;

        const float x1 = p.x * cy - p.z * sy;
        const float z1 = p.x * sy + p.z * cy;

        const float y2 = p.y * cx - z1 * sx;
        const float z2 = p.y * sx + z1 * cx;

        p.x = x1;
        p.y = y2;
        p.z = z2 + 4.0f;

        out->position = glm::vec4(p.x, p.y, p.z,p.z);
        out->inv_w = 1.0f / p.z;

        out->varyings[0] = color.r;
        out->varyings[1] = color.g;
        out->varyings[2] = color.b;
        out->flat_mask = ~0b111;
        out->used_mask = 0b111;
    }

    bool frshader(const Shader::FR_ShaderIN *In, Shader::FS_ShaderOut *out, void *uniform) {
        //const auto D = static_cast<uint8_t>(std::clamp(std::fract(In->depth), 0.0f, 1.0f) * 255);
        const auto R = static_cast<uint8_t>(In->varyings[0] * 255);
        const auto G = static_cast<uint8_t>(In->varyings[1] * 255);
        const auto B = static_cast<uint8_t>(In->varyings[2] * 255);

        out->color = (R << 16) | (G << 8) | B;

        return true;
    }
}