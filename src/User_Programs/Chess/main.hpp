#pragma once
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"

namespace Chess {
    extern OpenPL::Framebuffer fr;

    struct uniforms {
        uint32_t w;
        uint32_t h;
        float angle;
    };

    void main(int argc, char** argv);

    bool fps();
    extern volatile uint32_t frames;
    extern volatile uint64_t last_tick;
}
