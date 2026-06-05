#include "OpenPL.hpp"
#include "std/mem_common.hpp"
#include "Drivers/GPU/framebuffer.hpp"
#include "kernel/system.hpp"

namespace OpenPL {
    bool Context::bind_pipeline(const Pipeline p) {
        pipeline = p;
        return true;
    }

    bool Context::bind_vertex_buffer(void *buffer, uint64_t size) {
        if (size == 0) return false;
        vertex_Buffer = buffer;
        vertex_Buffer_size = size;
        return true;
    }

    bool Context::bind_framebuffer(const Framebuffer &f) {
        if (!f.framebuffer || f.bpp == 0 || f.height == 0 || f.width == 0) return false;
        framebuffer = f;
        return true;
    }

    void Context::Clear(const uint32_t Color) {
        uint32_t* fb = framebuffer.framebuffer;
        uint32_t w = framebuffer.width;
        uint32_t h = framebuffer.height;

        for (int y = 0; y < framebuffer.height; y++) {
            for (int x = 0; x < framebuffer.width; x++) {
                fb[x+(y*w)] = Color+x*y;
                //float cx = (x - w * 0.5f) * 3.0f / w;
                //float cy = (y - h * 0.5f) * 2.0f / h;
//
                //float zx = 0;
                //float zy = 0;
//
                //int i = 0;
                //const int maxIter = 50;
//
                //while (zx*zx + zy*zy < 4.0f && i < maxIter) {
//
                //    float xt = zx*zx - zy*zy + cx;
                //    zy = 2*zx*zy + cy;
                //    zx = xt;
//
                //    i++;
                //}
//
                //uint8_t color = (i * 255) / maxIter;
//
                //fb[x + y*w] =
                //    (0xFF << 24) |
                //    (color << 16);
            }
        }
    }

    void Context::Draw() {
        // soon
    }

    void Context::Swap() {
        auto info = systemPL::fb.get_screen_info();

        // Fixed point 16.16
        uint32_t step_x = (framebuffer.width << 16) / info.width;
        uint32_t step_y = (framebuffer.height << 16) / info.height;

        uint32_t fy = 0;
        for (uint32_t y = 0; y < info.height; y++) {

            // fixedpoint to int
            const uint32_t src_y = fy >> 16;
            fy += step_y;

            auto* dst_row = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(info.base) + y * info.pitch);

            uint32_t fx = 0;
            for (uint32_t x = 0; x < info.width; x++) {

                // fixedpoint to int
                const uint32_t src_x = fx >> 16;
                fx += step_x;

                dst_row[x] = framebuffer.framebuffer[src_y * framebuffer.width + src_x];
            }
        }
    }
}
