#include "arch/x86_64/syscall/syscall.h"
#include "libs/std/types.hpp"
#include "libs/std/mem_common.hpp"
#include "std/printf.hpp"
#include "kernel/system.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/heap.hpp"
#include "Drivers/Keyboard.hpp"
#include "Drivers/PCI.hpp"
#include "Drivers/vga.h"
#include "std/string.h"
#include "kernel/linker_info.hpp"

struct Command {
    const char *name;

    void (*func)(int argc, char** argv);
};

inline uint64_t range(void *a, void *b) {
    return reinterpret_cast<uint64_t>(b) - reinterpret_cast<uint64_t>(a);
}

void list_commands(int argc, char** argv);

Command commands[10] = {
    {"help", list_commands},
    {
        "clear", [](int argc, char** argv) {
            systemPL::fb.clear();
        }
    },
    {
        "poweroff", [](int argc, char** argv) {
            std::printf("&c\tShutting down in 5s (press ENTER to cancel!)\n");
            if (!Time::WaitForKey(5000, kb::key_code::KEY_ENTER)) {
                asm volatile("outw %0, %1" : : "a"(static_cast<uint16_t>(0x2000)), "Nd"(static_cast<uint16_t>(0x604)));
                // QEMU only
                std::printf("&4Unable to shut down try shutting down manually\n");
            } else {
                std::printf("&a\tShutdown Canceled!\n");
            }
        }
    },
    {
        "sleep", [](int argc, char** argv) {
            uint64_t wait = 1;
            uint8_t unit = true; // ms, s
            bool us = false;
            if (argc > 1) {
                for (int i = 1; i < argc; i++) {
                    if (std::str_cmp(argv[i], "-h")) {
                        std::printf("&7Usage: &fsleep &e[OPTIONS]\n\n");
                        std::printf("&eOption     &fMeaning\n");
                        std::printf("&b-us        &7Micro-seconds mode\n");
                        std::printf("&b-ms        &7Mili-seconds mode\n");
                        std::printf("&b-s         &7Seconds mode\n");
                        std::printf("&b-t TIME    &7Time to wait\n");
                        std::printf("&b-h         &7This text\n");
                        return;
                    } else if (std::str_cmp(argv[i], "-us")) {
                        us = true;
                        unit = false;
                    } else if (std::str_cmp(argv[i], "-ms")) {
                        unit = false;
                    } else if (std::str_cmp(argv[i], "-s")) {
                        unit = true;
                    } else if (std::str_cmp(argv[i], "-t")) {
                        if (i+1 < argc) {
                            wait = std::str_to_int(argv[i+1]);
                        }
                    }
                }
            }
            std::printf("&a\tSleeping for &f%l ", std::Output::std_out, wait);
            auto unit_str = unit ? "seconds" : "mili-seconds";
            if (us)
                unit_str = "micro-seconds";
            std::printf("&a%s\n", std::Output::std_out, unit_str);
            auto time = unit ? wait * 1000000 : wait * 1000;
            if (us)
                time = wait;
            sys_sleep(time);
        }
    },
    {
        "heap", [](int argc, char** argv) {
            bool show_all = false;
            if (argc > 0) {
                for (int i = 0; i < argc; i++) {
                    if (std::str_cmp(argv[i], "-s")) {
                        show_all = false;
                    } else if (std::str_cmp(argv[i], "-l")) {
                        show_all = true;
                    } else if (std::str_cmp(argv[i], "-h")) {
                        std::printf("&7Usage: &fheap &e[OPTIONS]\n\n");
                        std::printf("&eOption     &fMeaning\n");
                        std::printf("&b-l         &7Show all information about heap\n");
                        std::printf("&b-s         &7Show only summary\n");
                        std::printf("&b-h         &7This text\n");
                        return;
                    }
                }
            }
            sys_heap_dump(show_all);
        }
    },
    {
        "pci", [](int argc, char** argv) {
            sys_PCI_TEST();
        }
    },
    {
        "size", [](int argc, char** argv) {
            auto kernel_size = range(&Linker::__kernel_start, &Linker::__kernel_end);
            auto text_size = range(&Linker::__kernel_text_start, &Linker::__kernel_text_end);
            auto rodata_size = range(&Linker::__kernel_rodata_start, &Linker::__kernel_rodata_end);
            auto data_size = range(&Linker::__kernel_data_start, &Linker::__kernel_data_end);
            auto bss_size = range(&Linker::__kernel_bss_start, &Linker::__kernel_bss_end);
            auto stack_size = range(&Linker::stack_bottom, &Linker::stack_top);
            auto user_stack_size = range(&Linker::user_stack_bottom, &Linker::user_stack_top);

            auto kernel_code_size = text_size + rodata_size;

            std::printf("&9\t.text &7size: &a%u%s \t&9.rodata &7size: &a%u%s\n", std::Output::std_out, text_size,
                        std::format_size(text_size), rodata_size, std::format_size(rodata_size));
            std::printf("&9\t.data &7size: &a%u%s \t&9.bss &7size: &a%u%s\n", std::Output::std_out, data_size,
                        std::format_size(data_size), bss_size, std::format_size(bss_size));
            std::printf("&9\t.stack &7size: &a%u%s \t&9.user_stack &7size: &a%u%s\n", std::Output::std_out, stack_size,
                        std::format_size(stack_size), user_stack_size, std::format_size(user_stack_size));
            std::printf("&b\tKernel Code &7size: &a%u%s\n\n", std::Output::std_out, kernel_code_size,
                        std::format_size(kernel_code_size));
            std::printf("&e\tTotal kernel &7size: &a%u%s\n", std::Output::std_out, kernel_size,
                        std::format_size(kernel_size));
        }
    },
    {
        "usb", [](int argc, char** argv) {
        }
    },
    {
        "colors", [](int argc, char** argv) {
            std::printf(
                "&0 &&00 &1 &&11 &2 &&22 &3 &&33 &4 &&44 &5 &&55 &6 &&66 &7 &&77 &8 &&88 &9 &&99 &a &&aa &b &&bb &c &&cc &d &&dd &e &&ee &f &&ff\n");
        }
    },
    {
        "partitions", [](int argc, char** argv) {
            sys_list_parts();
        }
    },
};

struct TextCommand {
    char buffer[256];
};

void list_commands(int argc, char** argv) {
    std::printf("&9\tCommands: &9%s", std::Output::std_out, commands[0].name);
    for (i32 i = 1; i < sizeof(commands) / sizeof(Command); ++i) {
        std::printf("&9, %s", std::Output::std_out, commands[i].name);
    }
    std::printf("\n");
}

mem::Ring_Buffer<TextCommand, 64> previous_commands;

extern "C" void user_space_main() {
    std::printf("\n&aPrintf(%/i %/u %/s %/x %/c %/l %/f) &c%i %u %s %x %c %l %f\n", std::Output::std_out, -6767, 6767, "LOL",
                0x00006677, 'j', 0x7FFFFFFFFFFFFFFF, 3.146767);
    std::printf("&f------------ &bPlum OS 64bit &f------------\n\n");
    std::printf("&aHello from user space!\n");

    list_commands(0, nullptr);

    std::printf("&fPlum-OS> ");

    static char buffer[256];
    static int i = 0;
    while (true) {
        sys_swap_framebuffer();
        const kb::key_code key = sys_get_key();
        const char key_char = kb::to_char(key);

        if (key == kb::key_code::KEY_BACKSPACE) {
            if (i == 0)
                continue;
            std::put_char('\b');
            i--;
            buffer[i] = '\0';
            continue;
        }

        if (key == kb::key_code::KEY_ENTER) {
            buffer[i] = '\0';
            std::put_char('\n');

            // Get Command Name from buffer
            char command_name[256] = {0};
            int k = 0;
            int j = 0;

            while (buffer[k] != '\0' && buffer[k] == ' ')
                k++;

            while (buffer[k] != '\0' && buffer[k] != ' ' && j < 255) {
                command_name[j++] = buffer[k++];
            }

            command_name[j] = '\0';

            // Get arguments from buffer
            char* args[32];
            int argc = 0;
            k = 0;

            while (buffer[k] != '\0') {
                while (buffer[k] == ' ') k++;

                if (buffer[k] == '\0') break;

                args[argc++] = &buffer[k];

                while (buffer[k] != '\0' && buffer[k] != ' ')
                    k++;
            }

            k = 0;
            while (buffer[k] != '\0') {
                if (buffer[k] != ' ') {
                    k++;
                } else {
                    buffer[k] = '\0';
                    k++;
                    while (buffer[k] == ' ') k++;
                }
            }

            bool found_command = false;
            for (auto &[name, func] : commands) {
                if (std::str_cmp(command_name, name)) {
                    func(argc, args);
                    found_command = true;
                    break;
                }
            }
            if (!found_command) {
                std::printf("&7\tUnknown command: &c%s \n", std::Output::std_out, buffer);
                sys_sleep(250); // i like that delay :)
            }

            TextCommand command {};
            mem::memmove(command.buffer, buffer, 256);
            previous_commands.push(command);

            std::printf("&fPlum-OS> ");
            i = 0;
            continue;
        }

        if (key == kb::key_code::KEY_UP) {
            TextCommand command {};
            previous_commands.pop(command);
            mem::memmove(buffer, command.buffer, 256);
            std::print(buffer);
        }

        if (key_char == 0) {
            continue;
        }

        if  (i >= 255) {
            continue;
        }

        std::put_char(kb::to_char(key));
        buffer[i++] = key_char;
        if (key == kb::key_code::KEY_TAB) {
            i += drivers::vga::TAB_SIZE - 1;
        }
    }
}
