#pragma once
#include "../libs/std/Ring_Buffer.hpp"

namespace kb {
    struct AsciiMap {
        char data[256];
    };

    extern mem::Ring_Buffer<uint8_t, 256> buf;

    enum class key_code : uint8_t {
        // ---- Base set (scancode = index) ----
        KEY_NULL       = 0x00,
        KEY_ESC        = 0x01,
        KEY_ALPHA_1    = 0x02,
        KEY_ALPHA_2    = 0x03,
        KEY_ALPHA_3    = 0x04,
        KEY_ALPHA_4    = 0x05,
        KEY_ALPHA_5    = 0x06,
        KEY_ALPHA_6    = 0x07,
        KEY_ALPHA_7    = 0x08,
        KEY_ALPHA_8    = 0x09,
        KEY_ALPHA_9    = 0x0A,
        KEY_ALPHA_0    = 0x0B,
        KEY_MINUS      = 0x0C,
        KEY_EQUALS     = 0x0D,
        KEY_BACKSPACE  = 0x0E,
        KEY_TAB        = 0x0F,
        KEY_Q          = 0x10,
        KEY_W          = 0x11,
        KEY_E          = 0x12,
        KEY_R          = 0x13,
        KEY_T          = 0x14,
        KEY_Y          = 0x15,
        KEY_U          = 0x16,
        KEY_I          = 0x17,
        KEY_O          = 0x18,
        KEY_P          = 0x19,
        KEY_LBRACKET   = 0x1A,
        KEY_RBRACKET   = 0x1B,
        KEY_ENTER      = 0x1C,
        KEY_LCTRL      = 0x1D,
        KEY_A          = 0x1E,
        KEY_S          = 0x1F,
        KEY_D          = 0x20,
        KEY_F          = 0x21,
        KEY_G          = 0x22,
        KEY_H          = 0x23,
        KEY_J          = 0x24,
        KEY_K          = 0x25,
        KEY_L          = 0x26,
        KEY_SEMICOLON  = 0x27,
        KEY_APOSTROPHE = 0x28,
        KEY_BACKTICK   = 0x29,
        KEY_LSHIFT     = 0x2A,
        KEY_BACKSLASH  = 0x2B,
        KEY_Z          = 0x2C,
        KEY_X          = 0x2D,
        KEY_C          = 0x2E,
        KEY_V          = 0x2F,
        KEY_B          = 0x30,
        KEY_N          = 0x31,
        KEY_M          = 0x32,
        KEY_COMMA      = 0x33,
        KEY_PERIOD     = 0x34,
        KEY_SLASH      = 0x35,
        KEY_RSHIFT     = 0x36,
        KEY_KP_STAR    = 0x37,
        KEY_LALT       = 0x38,
        KEY_SPACE      = 0x39,
        KEY_CAPSLOCK   = 0x3A,
        KEY_F1         = 0x3B,
        KEY_F2         = 0x3C,
        KEY_F3         = 0x3D,
        KEY_F4         = 0x3E,
        KEY_F5         = 0x3F,
        KEY_F6         = 0x40,
        KEY_F7         = 0x41,
        KEY_F8         = 0x42,
        KEY_F9         = 0x43,
        KEY_F10        = 0x44,
        KEY_NUMLOCK    = 0x45,
        KEY_SCROLLLOCK = 0x46,
        KEY_KP_7       = 0x47,
        KEY_KP_8       = 0x48,
        KEY_KP_9       = 0x49,
        KEY_KP_MINUS   = 0x4A,
        KEY_KP_4       = 0x4B,
        KEY_KP_5       = 0x4C,
        KEY_KP_6       = 0x4D,
        KEY_KP_PLUS    = 0x4E,
        KEY_KP_1       = 0x4F,
        KEY_KP_2       = 0x50,
        KEY_KP_3       = 0x51,
        KEY_KP_0       = 0x52,
        KEY_KP_PERIOD  = 0x53,
        // 0x54..0x56 unused
        KEY_F11        = 0x57,
        KEY_F12        = 0x58,

        // ---- Extended set (0xE0 prefix; stored as second_byte + 0x59) ----
        // Second byte 0x1C -> 0x75, increases from there.
        KEY_KP_ENTER   = 0x75,  // E0 1C
        KEY_RCTRL      = 0x76,  // E0 1D
        KEY_KP_SLASH   = 0x8E,  // E0 35
        KEY_RALT       = 0x91,  // E0 38
        KEY_HOME       = 0xA0,  // E0 47
        KEY_UP         = 0xA1,  // E0 48
        KEY_PGUP       = 0xA2,  // E0 49
        KEY_LEFT       = 0xA4,  // E0 4B
        KEY_RIGHT      = 0xA6,  // E0 4D
        KEY_END        = 0xA8,  // E0 4F
        KEY_DOWN       = 0xA9,  // E0 50
        KEY_PGDOWN     = 0xAA,  // E0 51
        KEY_INSERT     = 0xAB,  // E0 52
        KEY_DELETE     = 0xAC,  // E0 53
        KEY_LSUPER     = 0xB4,  // E0 5B
        KEY_RSUPER     = 0xB5,  // E0 5C
        KEY_MENU       = 0xB6,  // E0 5D
    };

    constexpr AsciiMap build_ascii_map() {
        AsciiMap m {};
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_1)]    = '1';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_2)]    = '2';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_3)]    = '3';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_4)]    = '4';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_5)]    = '5';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_6)]    = '6';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_7)]    = '7';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_8)]    = '8';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_9)]    = '9';
        m.data[static_cast<uint8_t>(key_code::KEY_ALPHA_0)]    = '0';
        m.data[static_cast<uint8_t>(key_code::KEY_MINUS)]      = '-';
        m.data[static_cast<uint8_t>(key_code::KEY_EQUALS)]     = '=';
        m.data[static_cast<uint8_t>(key_code::KEY_BACKSPACE)]  = '\b';
        m.data[static_cast<uint8_t>(key_code::KEY_TAB)]        = '\t';
        m.data[static_cast<uint8_t>(key_code::KEY_Q)]          = 'q';
        m.data[static_cast<uint8_t>(key_code::KEY_W)]          = 'w';
        m.data[static_cast<uint8_t>(key_code::KEY_E)]          = 'e';
        m.data[static_cast<uint8_t>(key_code::KEY_R)]          = 'r';
        m.data[static_cast<uint8_t>(key_code::KEY_T)]          = 't';
        m.data[static_cast<uint8_t>(key_code::KEY_Y)]          = 'y';
        m.data[static_cast<uint8_t>(key_code::KEY_U)]          = 'u';
        m.data[static_cast<uint8_t>(key_code::KEY_I)]          = 'i';
        m.data[static_cast<uint8_t>(key_code::KEY_O)]          = 'o';
        m.data[static_cast<uint8_t>(key_code::KEY_P)]          = 'p';
        m.data[static_cast<uint8_t>(key_code::KEY_LBRACKET)]   = '[';
        m.data[static_cast<uint8_t>(key_code::KEY_RBRACKET)]   = ']';
        m.data[static_cast<uint8_t>(key_code::KEY_ENTER)]      = '\n';
        m.data[static_cast<uint8_t>(key_code::KEY_A)]          = 'a';
        m.data[static_cast<uint8_t>(key_code::KEY_S)]          = 's';
        m.data[static_cast<uint8_t>(key_code::KEY_D)]          = 'd';
        m.data[static_cast<uint8_t>(key_code::KEY_F)]          = 'f';
        m.data[static_cast<uint8_t>(key_code::KEY_G)]          = 'g';
        m.data[static_cast<uint8_t>(key_code::KEY_H)]          = 'h';
        m.data[static_cast<uint8_t>(key_code::KEY_J)]          = 'j';
        m.data[static_cast<uint8_t>(key_code::KEY_K)]          = 'k';
        m.data[static_cast<uint8_t>(key_code::KEY_L)]          = 'l';
        m.data[static_cast<uint8_t>(key_code::KEY_SEMICOLON)]  = ';';
        m.data[static_cast<uint8_t>(key_code::KEY_APOSTROPHE)] = '\'';
        m.data[static_cast<uint8_t>(key_code::KEY_BACKTICK)]   = '`';
        m.data[static_cast<uint8_t>(key_code::KEY_BACKSLASH)]  = '\\';
        m.data[static_cast<uint8_t>(key_code::KEY_Z)]          = 'z';
        m.data[static_cast<uint8_t>(key_code::KEY_X)]          = 'x';
        m.data[static_cast<uint8_t>(key_code::KEY_C)]          = 'c';
        m.data[static_cast<uint8_t>(key_code::KEY_V)]          = 'v';
        m.data[static_cast<uint8_t>(key_code::KEY_B)]          = 'b';
        m.data[static_cast<uint8_t>(key_code::KEY_N)]          = 'n';
        m.data[static_cast<uint8_t>(key_code::KEY_M)]          = 'm';
        m.data[static_cast<uint8_t>(key_code::KEY_COMMA)]      = ',';
        m.data[static_cast<uint8_t>(key_code::KEY_PERIOD)]     = '.';
        m.data[static_cast<uint8_t>(key_code::KEY_SLASH)]      = '/';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_STAR)]    = '*';
        m.data[static_cast<uint8_t>(key_code::KEY_SPACE)]      = ' ';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_7)]       = '7';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_8)]       = '8';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_9)]       = '9';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_MINUS)]   = '-';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_4)]       = '4';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_5)]       = '5';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_6)]       = '6';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_PLUS)]    = '+';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_1)]       = '1';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_2)]       = '2';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_3)]       = '3';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_0)]       = '0';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_PERIOD)]  = '.';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_SLASH)]   = '/';
        m.data[static_cast<uint8_t>(key_code::KEY_KP_ENTER)]   = '\n';
        return m;
    }

    inline constexpr AsciiMap ascii_map_storage = build_ascii_map();
    inline constexpr const char* ascii_map = ascii_map_storage.data;

    key_code get_char();
    key_code read_char();

    char to_char(key_code code);
}
