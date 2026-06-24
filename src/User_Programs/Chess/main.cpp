#include "main.hpp"

#include "arch/x86_64/syscall/syscall.h"
#include "std/printf.hpp"
#include "std/types.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/heap.hpp"
#include "std/string.h"

namespace Chess {
    using namespace OpenPL;

    volatile uint32_t frames;
    volatile uint64_t last_tick;
    Framebuffer fr{};

    void main(const int argc, char** argv) {
        uint32_t w = 1024;
        uint32_t h = 768;
        uint32_t bpp = 32;
        if (argc > 1 && argv) {
            for (int i = 1; i < argc; i++) {
                if (std::str_cmp(argv[i], "-w") && i+1 < argc) {
                    w = std::str_to_int(argv[i+1]);
                    if (w < 2) w = 2;
                    if (w > 3072) w = 3072;
                    i++;
                    continue;
                }
                if (std::str_cmp(argv[i], "-h") && i+1 < argc) {
                    h = std::str_to_int(argv[i+1]);
                    if (h < 2) h = 2;
                    if (h > 3072) h = 3072;
                    i++;
                    continue;
                }
                if (std::str_cmp(argv[i], "-bpp") && i+1 < argc) {
                    bpp = std::str_to_int(argv[i+1]);
                    if (bpp < 32) bpp = 32;
                    if (bpp > 32) bpp = 32;
                    i++;
                    continue;
                }
                if (std::str_cmp(argv[i], "--help")) {
                }
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

        // --------------------------------------
        // Setup OpenPL
        Context ctx{};

        // Configure Pipeline
        Pipeline pipeline = {};
        pipeline.Vertex_shader = vshader;
        pipeline.Fragment_shader = frshader;
        pipeline.cull_mode = CullingMode::BACK;
        pipeline.near_plane = 0.1f;
        pipeline.far_plane = 1000.0f;
        ctx.bind_pipeline(pipeline);

        // Configure Framebuffer
        fr.bpp = bpp;
        fr.width = w;
        fr.height = h;
        const auto raw_framebuffer = heap::malloc(w * h * bpp/8);
        const auto raw_depthbuffer = heap::malloc(w * h * sizeof(float));
        if (raw_framebuffer == nullptr || raw_depthbuffer == nullptr) {
            std::printf("&f[ CHESS ] &cNULLPTR in framebuffer!\n");
            heap::free(raw_framebuffer); heap::free(raw_depthbuffer);
            return;
        }
        fr.framebuffer = static_cast<uint32_t *>(raw_framebuffer);
        fr.depthbuffer = static_cast<float *>(raw_depthbuffer);
        ctx.bind_framebuffer(fr);

        float vertices[] = {
            // Front
            -0.5f,-0.5f, 0.5f, 1,0,0,
             0.5f,-0.5f, 0.5f, 1,1,0,
             0.5f, 0.5f, 0.5f, 1,1,1,

            -0.5f,-0.5f, 0.5f, 1,0,0,
             0.5f, 0.5f, 0.5f, 1,1,1,
            -0.5f, 0.5f, 0.5f, 1,0,1,

            // Back
             0.5f,-0.5f,-0.5f, 0,0,1,
            -0.5f,-0.5f,-0.5f, 0,1,1,
            -0.5f, 0.5f,-0.5f, 1,0,1,

             0.5f,-0.5f,-0.5f, 0,0,1,
            -0.5f, 0.5f,-0.5f, 1,0,1,
             0.5f, 0.5f,-0.5f, 1,1,1,

            // Left
            -0.5f,-0.5f,-0.5f, 0,1,1,
            -0.5f,-0.5f, 0.5f, 1,0,0,
            -0.5f, 0.5f, 0.5f, 1,0,1,

            -0.5f,-0.5f,-0.5f, 0,1,1,
            -0.5f, 0.5f, 0.5f, 1,0,1,
            -0.5f, 0.5f,-0.5f, 0,0,1,

            // Right
             0.5f,-0.5f, 0.5f, 1,1,0,
             0.5f,-0.5f,-0.5f, 0,0,1,
             0.5f, 0.5f,-0.5f, 1,1,1,

             0.5f,-0.5f, 0.5f, 1,1,0,
             0.5f, 0.5f,-0.5f, 1,1,1,
             0.5f, 0.5f, 0.5f, 0,1,0,

            // Top
            -0.5f, 0.5f, 0.5f, 1,0,1,
             0.5f, 0.5f, 0.5f, 0,1,0,
             0.5f, 0.5f,-0.5f, 1,1,1,

            -0.5f, 0.5f, 0.5f, 1,0,1,
             0.5f, 0.5f,-0.5f, 1,1,1,
            -0.5f, 0.5f,-0.5f, 0,0,1,

            // Bottom
            -0.5f,-0.5f,-0.5f, 0,1,1,
             0.5f,-0.5f,-0.5f, 0,0,1,
             0.5f,-0.5f, 0.5f, 1,1,0,

            -0.5f,-0.5f,-0.5f, 0,1,1,
             0.5f,-0.5f, 0.5f, 1,1,0,
            -0.5f,-0.5f, 0.5f, 1,0,0,
        };

        // Configure VBO
        ctx.bind_vertex_buffer(reinterpret_cast<uint8_t *>(vertices), sizeof(vertices));
        ctx.set_vertex_attr_type(0, AttributeType::ATTR_VEC3);
        ctx.set_vertex_attr_type(1, AttributeType::ATTR_VEC3);

        uniforms uni = {0.0f, 0.0f};
        int color;
        frames = 0;
        last_tick = Time::tick;

        // Main Loop
        while (true) {

            color++;
            ctx.Clear(color);
            uni.pitch += 0.01;
            uni.yaw += 0.05;

            for (int y = -8; y < 8; y++) {
                for (int x = -8; x < 8; x++) {
                    for (int z = -8; z < 8; z++) {
                        uni.offset = glm::vec3(x, y, z);
                        ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni)); // Any change to uniforms/VBO wont appear instantly you need to force it
                        ctx.Draw(PrimitiveType::TRIANGLES, 0, 3*12);
                    }
                }
            }

            // Swap screen with framebuffer
            sys_openPL(&ctx, GL_SWAP);

            if (fps()) return;
        }
    }

    bool fps() {
        volatile uint64_t now = Time::tick;
        if (now - last_tick >= 100) {
            std::printf("\t\t\t&fFPS: &e%u     &e%f&bs &fper frame\n", std::Output::std_out, frames, 1.0f / static_cast<float>(frames));
            std::printf("\t&fScreen Info: &7Unknown (too lazy)  &bScaling&f=&eNearest\n");
            std::printf("\t&fFramebuffer Info: &awidth&f=&e%u  &bheight&f=&e%u  &cbpp&f=&e%u\n", std::Output::std_out, fr.width, fr.height, fr.bpp);
            heap::free(fr.framebuffer);
            heap::free(fr.depthbuffer);
            return true;
        }
        frames++;
        return false;
    }
}
