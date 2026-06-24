#pragma once
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

namespace MyCraft {
    extern OpenPL::Framebuffer framebuffer;
    extern OpenPL::Pipeline pipeline;

    void vshader(const OpenPL::Shader::VS_ShaderIn *In, OpenPL::Shader::VS_ShaderOut *out, void *uniform);
    bool frshader(const OpenPL::Shader::FR_ShaderIN *In, OpenPL::Shader::FS_ShaderOut *out, void *uniform);

    struct Uniforms {
        float pitch;
        float yaw;
    };

    void main(int argc, char** argv);
}
