#pragma once
#include "std/types.hpp"
#include "std/math_types.hpp"
#include "std/vector.hpp"

namespace OpenPL {
    enum class AttributeType {
        ATTR_NONE,
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
        ATTR_MAT4,
    };

    enum class PrimitiveType {
        TRIANGLES,
        LINES,
    };

    enum class CullingMode {
        NONE,
        BACK,
        FRONT,
    };

    namespace Shader {
        constexpr uint8_t MAX_VARYINGS = 16;
        constexpr uint8_t MAX_ATTRIBUTES = 8;

        struct AttributeView {
            uint8_t *data;

            AttributeType type;
        };

        struct VS_ShaderIn {
            AttributeView attributes[MAX_ATTRIBUTES];
            size_t count;
        };

        struct VS_ShaderOut {
            glm::vec4 position = {};

            // interpolated values
            float varyings[MAX_VARYINGS] = {};
            uint16_t used_mask;
            uint16_t flat_mask;

            float inv_w = 1.0f;
        };

        typedef void (*vert_Shader)(const VS_ShaderIn *In, VS_ShaderOut *Out, void *uniforms);

        struct FR_ShaderIN {
            int x;
            int y;
            float depth;

            float *varyings;
        };

        struct FS_ShaderOut {
            uint32_t color;
            float depth;
        };

        typedef bool (*frag_Shader)(const FR_ShaderIN *In, FS_ShaderOut *Out, void *uniforms);
    }

    struct Pipeline {
        Shader::vert_Shader Vertex_shader;
        Shader::frag_Shader Fragment_shader;

        CullingMode cull_mode = CullingMode::BACK;

        float near_plane = 0.05f;
        float far_plane = 1000.0f;
    };

    struct Framebuffer {
        uint32_t *framebuffer;
        float *depthbuffer;
        uint32_t width;
        uint32_t height;
        uint8_t bpp;
    };

    #define GL_SWAP 0

    uint64_t size_of_attr(AttributeType type);

    inline float edge(const glm::vec2 A, const glm::vec2 B, const glm::vec2 P) {
        return (B.x - A.x) * (P.y - A.y) - (B.y - A.y) * (P.x - A.x);
    }

    class Context {
    public:
        bool bind_vertex_buffer(uint8_t *buffer, uint64_t size);
        bool set_vertex_attr_type(uint8_t idx, AttributeType type);
        bool set_uniform_ptr(uint8_t *ptr);
        bool bind_pipeline(Pipeline p);
        bool bind_framebuffer(const Framebuffer &f);

        void Clear(uint32_t Color);
        void Draw(PrimitiveType primitive, uint64_t start, uint64_t num_of_vert);

        void Swap();
    private:
        std::vector<Shader::VS_ShaderOut> vertex_cache;
        bool vbo_updated = true;
        uint32_t vbo_stride = 0;

        Framebuffer framebuffer{};

        Pipeline pipeline{};

        uint8_t *vertex_Buffer = nullptr;
        uint64_t vertex_Buffer_size = 0;
        AttributeType attribute_type[Shader::MAX_ATTRIBUTES] = { AttributeType::ATTR_NONE };
        uint8_t *uniform_ptr = nullptr;
    };
}
