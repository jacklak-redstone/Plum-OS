#pragma once
#include "std/types.hpp"
#include "std/math_types.hpp"

namespace OpenPL {
    enum AttributeType {
        ATTR_FLOAT,

        ATTR_VEC2,
        ATTR_VEC3,
        ATTR_VEC4,

        ATTR_INT,
        ATTR_IVEC2,
        ATTR_IVEC3,
        ATTR_IVEC4,

        ATTR_UINT,

        ATTR_MAT3,
        ATTR_MAT4
    };

    namespace Shader {
        constexpr uint16_t MAX_VARYINGS = 32;
        constexpr uint16_t MAX_ATTRIBUTES = 16;

        struct AttributeView {
            void *data;

            AttributeType type;
        };

        struct VS_ShaderIn {
            AttributeView attributes[MAX_ATTRIBUTES];
            size_t count;
        };

        struct VS_ShaderOut {
            glm::vec4 position;

            // interpolated values
            float varyings[MAX_VARYINGS];

            float inv_w;
        };

        typedef void (*vert_Shader)(VS_ShaderIn *In, VS_ShaderOut *Out, void *uniforms);

        struct FR_ShaderIN {
            float x;
            float y;
            float depth;

            float varyings[MAX_VARYINGS];
        };

        struct FS_ShaderOut {
            uint32_t color;
            float depth;

            bool discard;
        };

        typedef void (*frag_Shader)(FR_ShaderIN *In, FS_ShaderOut *Out, void *uniforms);
    }

    struct Pipeline {
        Shader::vert_Shader Vertex_shader;
        Shader::frag_Shader Fragment_shader;
    };

    struct Framebuffer {
        uint32_t *framebuffer;
        uint32_t width;
        uint32_t height;
        uint8_t bpp;
    };

    #define GL_SWAP 0

    class Context {
    public:
        bool bind_vertex_buffer(void *buffer, uint64_t size);
        bool bind_pipeline(Pipeline p);
        bool bind_framebuffer(const Framebuffer &f);

        void Clear(uint32_t Color);
        void Draw();

        void Swap();
    private:
        Framebuffer framebuffer{};

        Pipeline pipeline{};

        void *vertex_Buffer = nullptr;
        uint64_t vertex_Buffer_size = 0;
    };
}
