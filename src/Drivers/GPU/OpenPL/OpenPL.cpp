#include "OpenPL.hpp"
#include "std/mem_common.hpp"
#include "Drivers/GPU/framebuffer.hpp"
#include "kernel/system.hpp"
#include "std/math.hpp"

namespace OpenPL {
    bool Context::bind_pipeline(const Pipeline p) {
        pipeline = p;
        return true;
    }

    bool Context::bind_vertex_buffer(uint8_t *buffer, const uint64_t size) {
        if (size == 0) return false;
        vertex_Buffer = buffer;
        vertex_Buffer_size = size;
        vbo_updated = true;
        return true;
    }

    bool Context::bind_framebuffer(const Framebuffer &f) {
        if (!f.framebuffer || f.bpp == 0 || f.height == 0 || f.width == 0) return false;
        framebuffer = f;
        return true;
    }

    void Context::Clear(const uint32_t Color) {
        uint32_t* fb = framebuffer.framebuffer;
        float* db = framebuffer.depthbuffer;
        const uint32_t w = framebuffer.width;
        const uint32_t h = framebuffer.height;

        mem::memset32(fb, Color, h*w);
        mem::memset32(reinterpret_cast<uint32_t *>(db), 0x3f800000, h*w); // trust 0x3f800000 is 1.0f
    }

    bool Context::set_vertex_attr_type(const uint8_t attribute, const AttributeType type) {
        if (attribute < 0 || attribute >= Shader::MAX_ATTRIBUTES) return false;
        attribute_type[attribute] = type;
        return true;
    }

    bool Context::set_uniform_ptr(uint8_t *ptr) {
        if (ptr == nullptr) return false;
        uniform_ptr = ptr;
        return true;
    }

    uint64_t size_of_attr(const AttributeType type) {
        switch (type) {
            case AttributeType::ATTR_NONE: return 0;
            case AttributeType::ATTR_FLOAT: return sizeof(float);
            case AttributeType::ATTR_INT: return sizeof(int);
            case AttributeType::ATTR_UINT: return sizeof(unsigned int);
            case AttributeType::ATTR_VEC2: return sizeof(glm::vec2);
            case AttributeType::ATTR_VEC3: return sizeof(glm::vec3);
            case AttributeType::ATTR_VEC4: return sizeof(glm::vec4);
            case AttributeType::ATTR_IVEC2: return sizeof(glm::ivec2);
            case AttributeType::ATTR_IVEC3: return sizeof(int)*3;
            case AttributeType::ATTR_IVEC4: return sizeof(int)*4;
            case AttributeType::ATTR_MAT3: return sizeof(glm::vec3)*3;
            case AttributeType::ATTR_MAT4: return sizeof(glm::vec4)*4;
            default: return 0;
        }
    }

    void Context::Draw(PrimitiveType primitive, uint64_t start, const uint64_t num_of_vert) {
            if (num_of_vert == 0) return;
            if (start >= vertex_Buffer_size) return;
            if (start+num_of_vert-1 >= vertex_Buffer_size) return;

            // Vertex Shader Pass
            if (vbo_updated) {
                uint8_t *adr = vertex_Buffer;
                if (vertex_cache.capacity() < num_of_vert)
                    vertex_cache.reserve(num_of_vert);
                vertex_cache.clear();
                Shader::VS_ShaderIn in = {};
                Shader::VS_ShaderOut out = {};

                for (int i = start; i < start+num_of_vert; i++) {

                    for (int idx = 0; idx < Shader::MAX_ATTRIBUTES; idx++) {

                        if (attribute_type[idx] == AttributeType::ATTR_NONE) {
                            continue;
                        }

                        in.attributes[idx].type = attribute_type[idx];
                        in.attributes[idx].data = adr;
                        adr += size_of_attr(attribute_type[idx]);
                    }
                    pipeline.Vertex_shader(&in, &out, uniform_ptr);
                    vertex_cache.push_back(out);
                }
                vbo_updated = false;
            }

            // Primitive Assembly (Triangles)
            uint32_t* fb = framebuffer.framebuffer;
            float* db = framebuffer.depthbuffer;
            const uint32_t w = framebuffer.width;
            const uint32_t h = framebuffer.height;
            const auto w_float = static_cast<float>(framebuffer.width);
            const auto h_float = static_cast<float>(framebuffer.height);
            Shader::FR_ShaderIN fs_in = {};
            Shader::FS_ShaderOut fs_out = {};

            if (primitive == PrimitiveType::TRIANGLES) {
                for (int i = 0; i < vertex_cache.size(); i += 3) {
                    const auto& p1 = vertex_cache[i];
                    const auto& p2 = vertex_cache[i+1];
                    const auto& p3 = vertex_cache[i+2];

                    const auto p1_NDC = p1.position.get_v3() * glm::vec3(p1.inv_w);
                    const auto p2_NDC = p2.position.get_v3() * glm::vec3(p2.inv_w);
                    const auto p3_NDC = p3.position.get_v3() * glm::vec3(p3.inv_w);

                    const glm::vec2 p1_screen = {((p1_NDC.x * 0.5f + 0.5f) * w_float), ((1.0f - (p1_NDC.y * 0.5f + 0.5f)) * h_float)};
                    const glm::vec2 p2_screen = {((p2_NDC.x * 0.5f + 0.5f) * w_float), ((1.0f - (p2_NDC.y * 0.5f + 0.5f)) * h_float)};
                    const glm::vec2 p3_screen = {((p3_NDC.x * 0.5f + 0.5f) * w_float), ((1.0f - (p3_NDC.y * 0.5f + 0.5f)) * h_float)};

                    float area = edge(p1_screen, p2_screen, p3_screen);

                    if (pipeline.cull_mode == CullingMode::BACK) {
                        if (area < 0) continue;
                    } else if (pipeline.cull_mode == CullingMode::FRONT) {
                        if (area > 0) continue;
                    } else if (pipeline.cull_mode == CullingMode::NONE) {
                    }

                    const float invArea = 1.0f / area;

                    const int x_min = std::max(static_cast<int>(std::min(std::min(p1_screen.x, p2_screen.x), p3_screen.x)), 0);
                    const int y_min = std::max(static_cast<int>(std::min(std::min(p1_screen.y, p2_screen.y), p3_screen.y)), 0);
                    const int x_max = std::min(static_cast<int>(std::max(std::max(p1_screen.x, p2_screen.x), p3_screen.x)), static_cast<int>(w));
                    const int y_max = std::min(static_cast<int>(std::max(std::max(p1_screen.y, p2_screen.y), p3_screen.y)), static_cast<int>(h));

                    const glm::vec3 ABC1 = {p2_screen.y - p3_screen.y, p3_screen.x - p2_screen.x, p2_screen.x * p3_screen.y - p2_screen.y * p3_screen.x};
                    const glm::vec3 ABC2 = {p3_screen.y - p1_screen.y, p1_screen.x - p3_screen.x, p3_screen.x * p1_screen.y - p3_screen.y * p1_screen.x};
                    const glm::vec3 ABC3 = {p1_screen.y - p2_screen.y, p2_screen.x - p1_screen.x, p1_screen.x * p2_screen.y - p1_screen.y * p2_screen.x};

                    float w1_row = ABC1.x*x_min + ABC1.y*y_min + ABC1.z;
                    float w2_row = ABC2.x*x_min + ABC2.y*y_min + ABC2.z;
                    float w3_row = ABC3.x*x_min + ABC3.y*y_min + ABC3.z;

                    const float* v0 = p1.varyings;
                    const float* v1 = p2.varyings;
                    const float* v2 = p3.varyings;

                    float vars_dx[Shader::MAX_VARYINGS];
                    float vars_dy[Shader::MAX_VARYINGS];
                    float vars_row[Shader::MAX_VARYINGS];
                    float vars[Shader::MAX_VARYINGS];
                    fs_in.varyings = vars;

                    if (p1.used_mask != 0) {
                        // Flat varyings (dont change so can be done before raster loop)
                        uint16_t mask = p1.used_mask & p1.flat_mask;
                        while (mask) {
                            int count = __builtin_ctz(mask);
                            mask &= mask - 1;

                            vars_dx[count] = 0;
                            vars_dy[count] = 0;
                            vars_row[count] = v0[count];
                            vars[count] = v0[count];
                        }

                        // Smooth varyings
                        mask = p1.used_mask & ~p1.flat_mask;
                        while (mask) {
                            int count = __builtin_ctz(mask);
                            mask &= mask - 1;

                            vars_dx[count] = (ABC1.x*v0[count] + ABC2.x*v1[count] + ABC3.x*v2[count]) * invArea;
                            vars_dy[count] = (ABC1.y*v0[count] + ABC2.y*v1[count] + ABC3.y*v2[count]) * invArea;
                            vars_row[count] = (w1_row*v0[count] + w2_row*v1[count] + w3_row*v2[count]) * invArea;
                        }
                    }

                    const float depth_dx = (ABC1.x*p1.position.z + ABC2.x*p2.position.z + ABC3.x*p3.position.z) * invArea;
                    const float depth_dy = (ABC1.y*p1.position.z + ABC2.y*p2.position.z + ABC3.y*p3.position.z) * invArea;
                    float depth_row = (w1_row*p1.position.z + w2_row*p2.position.z + w3_row*p3.position.z) * invArea;

                    int last_used_varying = 31 - __builtin_clz(p1.used_mask & ~p1.flat_mask);
                    if (p1.used_mask == 0) last_used_varying = -1;

                    // Raster Loop
                    // TODO Tile based rendering
                    for (int y = y_min; y < y_max; y++) {
                        uint32_t idx = (x_min + y * w) - 1; // -1 bc we increment at start
                        // - ABCx.x bc we increment at start
                        float w1 = w1_row - ABC1.x;
                        float w2 = w2_row - ABC2.x;
                        float w3 = w3_row - ABC3.x;
                        float depth = depth_row - depth_dx;
                        for (int v_idx = 0; v_idx <= last_used_varying; v_idx++) {
                            vars[v_idx] = vars_row[v_idx] - vars_dx[v_idx];
                        }
                        for (int x = x_min; x < x_max; x++) {
                            // At start bc if at end it would sometimes be skipped if not in triangle
                            idx++;
                            w1 += ABC1.x;
                            w2 += ABC2.x;
                            w3 += ABC3.x;
                            depth += depth_dx;
                            for (int v_idx = 0; v_idx <= last_used_varying; v_idx++) {
                                vars[v_idx] += vars_dx[v_idx];
                            }

                            // Depth test
                            if (depth > db[idx]) continue;

                            // In triangle test             Back                                 Front
                            const bool inside = (w1 >= 0 && w2 >= 0 && w3 >= 0) || (w1 <= 0 && w2 <= 0 && w3 <= 0);

                            if (!inside) continue;

                            // Fragment shader Call
                            fs_in.x = x; fs_in.y = y; fs_in.depth = depth;

                            if (!pipeline.Fragment_shader(&fs_in, &fs_out, uniform_ptr)) continue;

                            // Write to framebuffers
                            fb[idx] = fs_out.color;
                            db[idx] = depth;
                        }
                        w1_row += ABC1.y;
                        w2_row += ABC2.y;
                        w3_row += ABC3.y;
                        depth_row += depth_dy;
                        for (int v_idx = 0; v_idx <= last_used_varying; v_idx++) {
                            vars_row[v_idx] += vars_dy[v_idx];
                        }
                    }
                }
            }
        }

    void Context::Swap() {
        const auto info = systemPL::fb.get_screen_info();
        const auto fb = framebuffer.framebuffer;

        // Fixed point 16.16
        const uint32_t step_x = (framebuffer.width << 16) / info.width;
        const uint32_t step_y = (framebuffer.height << 16) / info.height;

        uint32_t fy = 0;
        for (uint32_t y = 0; y < info.height; y++) {

            // fixedpoint to int
            const uint32_t *fb_row = fb + (fy >> 16) * framebuffer.width;
            fy += step_y;

            auto* dst_row = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(info.base) + y * info.pitch);

            uint32_t fx = 0;
            for (uint32_t x = 0; x < info.width; x++) {

                // fixedpoint to int
                const uint32_t src_x = fx >> 16;
                fx += step_x;

                dst_row[x] = fb_row[src_x];
            }
        }
    }
}
