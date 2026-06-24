#include "main.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "std/math_types.hpp"
#include "std/trigonometry.hpp"

namespace MyCraft {
    using namespace OpenPL;

    void vshader(const Shader::VS_ShaderIn *In, Shader::VS_ShaderOut *out, void *uniform) {
        const auto uni = *static_cast<Uniforms *>(uniform);
        const auto pos = *reinterpret_cast<glm::vec3 *>(In->attributes[0].data);
        const auto color = *reinterpret_cast<glm::vec3 *>(In->attributes[1].data);

        const float sy = std::sin(uni.pitch);
        const float cy = std::cos(uni.pitch);

        const float sx = std::sin(uni.yaw);
        const float cx = std::cos(uni.yaw);

        glm::vec3 p = pos;

        const float x1 = p.x * cy - p.z * sy;
        const float z1 = p.x * sy + p.z * cy;

        const float y2 = p.y * cx - z1 * sx;
        const float z2 = p.y * sx + z1 * cx;

        p.x = x1;
        p.y = y2;
        p.z = z2 + 1.0f;

        out->position = glm::vec4(p, p.z);
        out->inv_w = 1.0f / p.z;

        out->varyings[0] = color.r;
        out->varyings[1] = color.g;
        out->varyings[2] = color.b;

        out->used_mask = 0b111;
        out->flat_mask = ~0b111;
    }

    bool frshader(const Shader::FR_ShaderIN *In, Shader::FS_ShaderOut *out, void *uniform) {
        const auto R = static_cast<uint8_t>(In->varyings[0] * 255);
        const auto G = static_cast<uint8_t>(In->varyings[1] * 255);
        const auto B = static_cast<uint8_t>(In->varyings[2] * 255);

        out->color = (R << 16) | (G << 8) | B;

        return true;
    }
}
