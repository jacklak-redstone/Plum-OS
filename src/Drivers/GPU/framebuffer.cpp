// ReSharper disable CppDFANullDereference
#include "framebuffer.hpp"

#include "glyphs.h"
#include "arch/x86_64/Common/Common.hpp"
#include "Drivers/vga.h"
#include "kernel/log.h"
#include "kernel/Paging.hpp"
#include "kernel/Memory/heap.hpp"
#include "kernel/Memory/mem_helper.h"
#include "std/mem_common.hpp"

namespace framebuffer {
    constexpr i32 BACKGROUND_COLOR = 0x0F0F0F;

    void framebuffer::init(framebuffer_info framebuffer) {
        if (!framebuffer.base)
            return;
        info = framebuffer;

        front_buffer = static_cast<u32*>(framebuffer.base);
        back_buffer = static_cast<u32*>(heap::malloc(info.size));
        Paging::Map_memory_vp(
            reinterpret_cast<u64>(framebuffer.base),
            to_physical(reinterpret_cast<u64>(framebuffer.base)),
            framebuffer.size,
            Paging::Profile::VramWC
        );
        clear(BACKGROUND_COLOR);
        height_in_chars = info.height / 16;
        width_in_chars = info.width / 8;
        initialized = true;

        swap();
    }

    void framebuffer::swap() {
        if (!initialized || !is_dirty)
            return;
        mem::memcpy(front_buffer, back_buffer, info.size);
        is_dirty = false;
    }

    void framebuffer::clear(const u32 color) {
        const bool old_flag = x64::get_INT_flag();
        x64::set_INT_flag(false);
        mem::memset32(back_buffer, color, info.height * info.pixels_in_scanline);
        x64::set_INT_flag(old_flag);
        is_dirty = true;
    }

    void framebuffer::set_pixel(const u32 x, const u32 y, const u32 color) const {
        back_buffer[(y * info.pixels_in_scanline) + x] = color;
    }

    void framebuffer::put_char_at(const char c, const u32 x, const u32 y, const u32 color) {
        if (!initialized)
            return;
        is_dirty = true;
        const auto pixel_x = x * 8;
        const auto pixel_y = y * 16;
        const u8* glyph = glyph::get(c);
        for (int row = 0; row < 16; row++) {
            if (pixel_y + row >= info.height)
                break;
            for (int col = 0; col < 8; col++) {
                if (pixel_x + col >= info.width)
                    break;
                const bool set = glyph[row] & (0b10000000 >> col);
                set_pixel(pixel_x + col, pixel_y + row, set ? color : BACKGROUND_COLOR);
            }
        }
    }

    void framebuffer::put_char(const char c, const u32 color) {
        if (!initialized)
            return;
        if (c == '\t') { // Tab
            inc_cursor(drivers::vga::TAB_SIZE);
            put_char(' ', color);
            return;
        }
        if (c == '\n') { // Return
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= height_in_chars) {
                scroll();
                cursor_y = height_in_chars - 1;
            }
            return;
        }
        if (c == '\b') { // Backspace
            put_char_at(' ', cursor_x, cursor_y, color); // just in case
            dec_cursor(1);
            put_char_at(' ', cursor_x, cursor_y, color);
            return;
        }

        put_char_at(c, cursor_x, cursor_y, color);

        inc_cursor(1);
    }

    void framebuffer::scroll(const u32 lines) {
        if (!initialized) {
            log::error("[ FB ] Tried to scrool with uninitialized framebuffer");
            return;
        }
        if (lines == 0) {
            log::error("[ FB ] Tried to scroll 0 lines");
            return;
        }

        constexpr uint32_t px_per_line = 16;
        uint32_t scroll_px = lines * px_per_line;

        if (scroll_px >= info.height) {
            clear(BACKGROUND_COLOR);
            return;
        }

        const bool old_flag = x64::get_INT_flag();
        x64::set_INT_flag(false);

        const uint32_t pitch = info.pixels_in_scanline;

        mem::memcpy(back_buffer,back_buffer + scroll_px * pitch,(info.height - scroll_px) * pitch * sizeof(u32));
        mem::memset32(back_buffer + (info.height - scroll_px) * pitch,BACKGROUND_COLOR,scroll_px * pitch);

        is_dirty = true;
        x64::set_INT_flag(old_flag);
    }
}
