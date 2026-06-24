#include "main.hpp"
#include "Drivers/GPU/OpenPL/OpenPL.hpp"
#include "kernel/Memory/heap.hpp"
#include "arch/x86_64/syscall/syscall.h"

namespace MyCraft {
    using namespace OpenPL;

    Framebuffer framebuffer = {};
    Pipeline pipeline = {};

    void main(const int argc, char** argv) {
        // One of legacy resolutions
        const int width = 1024;
        const int height = 768;

        // Create a context
        Context ctx = {};

       // Now we can create a pipeline
        Pipeline pipeline = {};
        pipeline.Vertex_shader = vshader;
        pipeline.Fragment_shader = frshader;
        pipeline.near_plane = 0.05f;
        pipeline.far_plane = 1000.0f;
        pipeline.cull_mode = CullingMode::BACK;
        ctx.bind_pipeline(pipeline);

        // Creating Framebuffer and Depthbuffer
        framebuffer.bpp = 32;
        framebuffer.width = width;
        framebuffer.height = height;
        auto *raw_framebuffer = static_cast<uint32_t *>(heap::malloc(width * height * (framebuffer.bpp/8)));
        auto *raw_depthkbuffer = static_cast<float *>(heap::malloc(width*height * sizeof(float)));
        if (raw_framebuffer == nullptr || raw_depthkbuffer == nullptr) {
            heap::free(raw_framebuffer);
            heap::free(raw_depthkbuffer);
            return;
        }
        framebuffer.framebuffer = raw_framebuffer;
        framebuffer.depthbuffer = raw_depthkbuffer;
        ctx.bind_framebuffer(framebuffer);

        float mesh[] = {
            0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };

        ctx.bind_vertex_buffer(reinterpret_cast<uint8_t *>(mesh), sizeof(mesh));
        ctx.set_vertex_attr_type(0, AttributeType::ATTR_VEC3); // Position
        ctx.set_vertex_attr_type(1, AttributeType::ATTR_VEC3); // Color

        Uniforms uni = {};

        float p = 0.0f;
        float y = 0.0f;

        while (true) {
            ctx.Clear(0x303030);

            p += 0.01f;
            y += 0.001f;

            uni.pitch = p;
            uni.yaw = y;
            ctx.set_uniform_ptr(reinterpret_cast<uint8_t *>(&uni)); // We have to do this every time something changes in uniforms
            ctx.Draw(PrimitiveType::TRIANGLES, 0, 6);

            sys_openPL(&ctx, GL_SWAP);
        }
    }
}
