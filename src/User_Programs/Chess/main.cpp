#include "main.hpp"

#include "arch/x86_64/syscall/syscall.h"
#include "std/printf.hpp"
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/heap.hpp"
#include "std/string.h"
#include "std/trigonometry.hpp"
#include "std/math.hpp"

namespace Chess {
    using namespace OpenPL;

    volatile uint32_t frames;
    volatile uint64_t last_tick;
    Framebuffer fr{};

    void vshader(const Shader::VS_ShaderIn *In, Shader::VS_ShaderOut *out, void *uniforms) {
        const glm::vec3 pos = *reinterpret_cast<glm::vec3 *>(In->attributes[0].data);
        const glm::vec3 color = *reinterpret_cast<glm::vec3 *>(In->attributes[1].data);

        out->position = glm::vec4(pos, 1.0f);
        out->inv_w = 1.0f;
        out->varyings[0] = std::clamp(color.r, 0.0f, 1.0f);
        out->varyings[1] = std::clamp(color.g, 0.0f, 1.0f);
        out->varyings[2] = std::clamp(color.b, 0.0f, 1.0f);
        out->flat_mask = ~0b111;
        out->used_mask = 0b111;
    }

    bool frshader(const Shader::FR_ShaderIN *In, Shader::FS_ShaderOut *out, void *uniform) {
        //const auto D = static_cast<uint8_t>(std::clamp(In->depth, 0.0f, 1.0f) * 255);
        const auto R = static_cast<uint8_t>(In->varyings[0] * 255);
        const auto G = static_cast<uint8_t>(In->varyings[1] * 255);
        const auto B = static_cast<uint8_t>(In->varyings[2] * 255);

        out->color = (R << 16) | (G << 8) | B;

        return true;
    }

    void main(const int argc, char** argv) {
        uint32_t w = 1080;
        uint32_t h = 768;
        uint32_t bpp = 32;
        if (argc > 1 && argv) {
            for (int i = 1; i < argc; i++) {
                if (std::str_cmp(argv[i], "-w") && i+1 < argc) {
                    w = std::str_to_int(argv[i+1]);
                    if (w < 2) w = 2;
                    if (w > 3072) w = 3072;
                    i++;
                } else if (std::str_cmp(argv[i], "-h") && i+1 < argc) {
                    h = std::str_to_int(argv[i+1]);
                    if (h < 2) h = 2;
                    if (h > 3072) h = 3072;
                    i++;
                } else if (std::str_cmp(argv[i], "-bpp") && i+1 < argc) {
                    bpp = std::str_to_int(argv[i+1]);
                    if (bpp < 32) bpp = 32;
                    if (bpp > 32) bpp = 32;
                    i++;
                } else if (std::str_cmp(argv[i], "--help")) {
                    std::printf("&7Usage: &fchess &e[OPTIONS]\n\n");
                    std::printf("&eOption     &fMeaning\n");
                    std::printf("&b-w         &7Set width to custom value. Range from 2 to 3072 pixels\n");
                    std::printf("&b-h         &7Set heigh to custom value. Range from 2 to 3072 pixels\n");
                    std::printf("&b-bpp       &7Set bits-per-pixel to custom vallue. Range from 32 to 32 pixels\n");
                    std::printf("&b--help     &7This text\n");
                    std::printf("&fExample:&7 chess -w 32 -h 67 -bpp 32\n");
                    return;
                } else {
                    std::printf("&7Usage: &fchess &e[OPTIONS]\n\n");
                    std::printf("&eOption     &fMeaning\n");
                    std::printf("&b-w         &7Set width to custom value. Range from 2 to 3072 pixels\n");
                    std::printf("&b-h         &7Set heigh to custom value. Range from 2 to 3072 pixels\n");
                    std::printf("&b-bpp       &7Set bits-per-pixel to custom vallue. Range from 32 to 32 pixels\n");
                    std::printf("&b--help     &7This text\n");
                    std::printf("&fExample:&7 chess -w 32 -h 67 -bpp 32\n");
                    return;
                }
            }
        }
        Context ctx{};
        Pipeline pipeline{};
        pipeline.Vertex_shader = vshader;
        pipeline.Fragment_shader = frshader;
        pipeline.cull_mode = CullingMode::NONE;
        ctx.bind_pipeline(pipeline);

        fr.bpp = bpp;
        fr.width = w;
        fr.height = h;
        const auto raw = heap::malloc(w * h * bpp/8);
        const auto raw_depthbuffer = heap::malloc(w * h * sizeof(float));
        if (raw == nullptr) {
            std::printf("&f[ CHESS ] &cNULLPTR in framebuffer!\n");
            return;
        }
        fr.framebuffer = static_cast<uint32_t *>(raw);
        fr.depthbuffer = static_cast<float *>(raw_depthbuffer);

        ctx.bind_framebuffer(fr);

        float vertices[] = {
            // tri 1
            0.7f,  0.5f, 0.4f, 1.0f, 0.0f, 0.0f,
           -0.5f, -0.5f, 0.2f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.2f, 0.0f, 0.0f, 1.0f,

            // tri 2
            -0.3f, 0.6f, 1.0f, 1.0f, 1.0f, 0.0f,
            -0.5f, 0.2f, 1.0f, 1.0f, 0.0f, 0.0f,
            0.2f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f,

            // tri 3
             0.6f,  0.6f, 0.0f, 1.0f, 1.0f, 1.0f,
             0.3f,  0.2f, 0.1f, 0.0f, 0.0f, 0.0f,
             0.9f,  0.2f, 0.0f, 0.5f, 0.5f, 0.5f,

            // tri 4
            -0.6f, -0.2f, 0.0f, 1.0f, 0.0f, 1.0f,
           -0.9f, -0.6f, 0.0f, 1.0f, 0.0f, 0.0f,
           -0.3f, -0.6f, 0.1f, 0.0f, 1.0f, 1.0f,

            // tri 5
             0.6f, -0.2f, 0.2f, 0.6f, 0.2f, 0.6f,
             0.3f, -0.6f, 0.0f, 0.7f, 1.0f, 1.0f,
             0.9f, -0.6f, 0.0f, 0.1f, 0.8f, 0.1f,

            // tri 6
             0.0f,  0.0f, 0.0f, 0.1f, 0.0f, 0.3f,
             0.2f, -0.3f, 0.3f, 0.0f, 0.9f, 0.0f,
            -0.2f, -0.3f, 0.3f, 0.0f, 0.0f, 0.6f,

            // tri 7
            -0.2f,  0.3f, 0.6f, 0.0f, 0.1f, 0.2f,
            -0.4f,  0.0f, 0.0f, 0.2f, 0.8f, 0.0f,
             0.0f,  0.0f, 0.7f, 0.1f, 0.0f, 0.3f,

            // tri 8
             0.2f,  0.3f, 0.5f, 1.0f, 0.0f, 0.0f,
             0.0f,  0.0f, 0.5f, 0.5f, 0.0f, 0.0f,
             0.4f,  0.0f, 0.5f, 0.0f, 0.0f, 0.0f,

            // tri 9
            -0.8f,  0.0f, 0.1f, 0.0f, 1.0f, 0.0f,
            -0.6f, -0.4f, 0.1f, 0.0f, 0.5f, 0.0f,
            -1.0f, -0.4f, 0.1f, 0.0f, 0.0f, 0.0f,

            // tri 10
             0.8f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             1.0f, -0.4f, 0.2f, 0.0f, 0.0f, 0.5f,
             0.6f, -0.4f, 0.3f, 0.0f, 0.0f, 0.0f,
        };

        ctx.bind_vertex_buffer(reinterpret_cast<uint8_t *>(vertices), sizeof(vertices));
        ctx.set_vertex_attr_type(0, AttributeType::ATTR_VEC3);
        ctx.set_vertex_attr_type(1, AttributeType::ATTR_VEC3);

        uniforms uni = {w, h};
        ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni));

        u32 color = 0x0;
        frames = 0;
        last_tick = Time::tick;
        while (true) { // Main Loop

            ctx.Clear(color);
            color++;

            ctx.Draw(PrimitiveType::TRIANGLES, 0, 3*10);

            sys_openPL(&ctx, GL_SWAP);
            if (fps()) return;
        }
    }

    bool fps() {
        volatile uint64_t now = Time::tick;
        if (now - last_tick >= 100) {
            std::printf("\t\t\t&f Compiled with=&c-O0 huh it fixed now so nvm\n"); // bc -O1-3 broken
            std::printf("\t\t\t&fFPS: &e%u     &e%f&bs &fper frame\n", std::Output::std_out, frames, 1.0f / static_cast<float>(frames));
            std::printf("\t&fScreen Info: &7Unknown (too lazy)  &bScaling&f=&eNearest\n");
            std::printf("\t&fFramebuffer Info: &awidth&f=&e%u  &bheight&f=&e%u  &cbpp&f=&e%u\n", std::Output::std_out, fr.width, fr.height, fr.bpp);
            heap::free(fr.framebuffer);
            return true;
        }
        frames++;
        return false;
    }
}
