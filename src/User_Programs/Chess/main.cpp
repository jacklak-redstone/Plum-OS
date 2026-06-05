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
    constexpr float rect_size = 0.1f;

    void vshader(Shader::VS_ShaderIn *In, Shader::VS_ShaderOut *out, void *uniforms) {

    }

    void frshader(Shader::FR_ShaderIN *In, Shader::FS_ShaderOut *out, void *uniforms) {

    }

    void main(int argc, char** argv) {
        uint32_t w = 1080;
        uint32_t h = 768;
        uint32_t bpp = 32;
        if (argc > 1 && argv) {
            for (int i = 1; i < argc; i++) {
                if (std::str_cmp(argv[i], "-w") && i+1 < argc) {
                    w = std::str_to_int(argv[i+1]);
                    if (w < 2) w = 2;
                    if (w > 4096) w = 4096;
                } else if (std::str_cmp(argv[i], "-h") && i+1 < argc) {
                    h = std::str_to_int(argv[i+1]);
                    if (h < 2) h = 2;
                    if (h > 4096) h = 4096;
                } else if (std::str_cmp(argv[i], "-bpp") && i+1 < argc) {
                    bpp = std::str_to_int(argv[i+1]);
                    if (bpp < 32) bpp = 32;
                    if (bpp > 32) bpp = 32;
                } else if (std::str_cmp(argv[i], "--help")) {
                    std::printf("&7Usage: &fchess &e[OPTIONS]\n\n");
                    std::printf("&eOption     &fMeaning\n");
                    std::printf("&b-w         &7Set width to custom value. Range from 2 to 4096 pixels\n");
                    std::printf("&b-h         &7Set heigh to custom value. Range from 2 to 4096 pixels\n");
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
        ctx.bind_pipeline(pipeline);

        Framebuffer fr{};
        fr.bpp = bpp;
        fr.width = w;
        fr.height = h;
        fr.framebuffer = static_cast<uint32_t *>(heap::malloc(w * h * bpp/8));

        ctx.bind_framebuffer(fr);

        u32 color = 0xff;
        uint64_t frames = 0;
        uint64_t last_tick = Time::tick;
        while (true) {
            frames++;

            ctx.Clear(color);
            sys_openPL(&ctx, GL_SWAP);

            uint64_t now = Time::tick;
            if (now - last_tick >= 100) {
                std::printf("\t\t\t&f Compiled with=&c-O0\n", std::Output::std_out, frames); // bc -O1-3 broken
                std::printf("\t\t\t&7FPS: &f%u\n", std::Output::std_out, frames);
                std::printf("\t&fScreen Info: &7Unknown (too lazy)  &bScaling&f=&eNearest\n");
                std::printf("\t&fFramebuffer Info: &awidth&f=&e%u  &bheight&f=&e%u  &cbpp&f=&e%u\n", std::Output::std_out, fr.width, fr.height, fr.bpp);

                color = frames;
                frames = 0;
                last_tick = now;
                //sys_sleep(1000000); // to be able to see how many fps
                break;
            }
        }
    }
}
