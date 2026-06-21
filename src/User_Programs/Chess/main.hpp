#pragma once
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

namespace Chess {
    extern OpenPL::Framebuffer fr;

    void vshader(const OpenPL::Shader::VS_ShaderIn *In, OpenPL::Shader::VS_ShaderOut *out, void *uniform);
    bool frshader(const OpenPL::Shader::FR_ShaderIN *In, OpenPL::Shader::FS_ShaderOut *out, void *uniform);

    struct uniforms {
        float pitch;
        float yaw;
        glm::vec3 offset;
    };

    void main(int argc, char** argv);

    bool fps();
    extern volatile uint32_t frames;
    extern volatile uint64_t last_tick;
}
