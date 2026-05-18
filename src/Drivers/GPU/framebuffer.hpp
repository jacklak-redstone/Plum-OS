#pragma once
#include "std/types.hpp"

struct Framebuffer;

namespace framebuffer {
    struct screen_info {
        u32 width;
        u32 height;
    };

    struct framebuffer_info {
        void* base;
        u32 width;
        u32 height;
        u32 pixels_in_scanline;
        u64 size;
    };

    class framebuffer {
    public:
        framebuffer() = default;
        ~framebuffer() = default;
        void init(framebuffer_info framebuffer);

        void swap();
        void clear(u32 color = 0x0F0F0F);
        void set_pixel(u32 x, u32 y, u32 color) const;
        void set_dirty() { is_dirty = true; }

        void put_char_at(char c, u32 x, u32 y, u32 color);
        void put_char(char c, u32 color);

        void scroll(u32 lines = 1);

        i32 get_height_in_chars() const { return height_in_chars; }
        screen_info get_screen_info() const { return {info.width, info.height}; }
    private:
        void inc_cursor(const i32 amount) {
            cursor_x += amount;
            if (cursor_x >= width_in_chars) {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= height_in_chars) {
                    scroll();
                    cursor_y = height_in_chars - 1;
                }
            }
        }

        void dec_cursor(const i32 amount) {
            cursor_x -= amount;
            if (cursor_x < 0) {
                cursor_x = width_in_chars - 1;
                if (cursor_y > 0) {
                    cursor_y--;
                }
            }
        }

        bool initialized = false;
        bool is_dirty = true;
        i32 height_in_chars = 0;
        i32 width_in_chars = 0;

        i32 cursor_x = 0;
        i32 cursor_y = 0;
        u32* back_buffer {};
        u32* front_buffer {};
        framebuffer_info info {};
    };
}
