struct Framebuffer {
    void* base;
    unsigned long long size;
    unsigned int width;
    unsigned int height;
    unsigned int pixels_per_scanline;
};

extern "C" void _start(Framebuffer *fb) {
    auto* screen = static_cast<unsigned int *>(fb->base);

    for (unsigned int y = 0; y < fb->height; y++) {
        for (unsigned int x = 0; x < fb->width; x++)
            screen[y * fb->pixels_per_scanline + x] = 0x00FF0000;
    }

    while (true) {}
}