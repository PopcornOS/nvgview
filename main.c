#include "nvgif.h"

int pop_API pop_main(pop_Services* svc, int argc, CHAR16** argv) {
    if (argc < 2) {
        svc->println(svc, L"usage: nvgview file.nvg");
        return 1;
    }

    popg_GraphicsServices* sgfx = svc->sgfx;
    if (sgfx->init(sgfx) != pop_SUCCESS) {
        svc->println(svc, L"nvgview: failed to init graphics");
        return 1;
    }

    nvg_Image* img = nvg_decode_image(svc, argv[1]);
    if (!img) {
        svc->println(svc, L"nvgview: failed to decode image");
        sgfx->deinit(sgfx);
        return 1;
    }

    // Draw the image at (0,0) or centered
    int startX = (sgfx->w - img->width) / 2;
    int startY = (sgfx->h - img->height) / 2;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int src = (y * img->width + x) * 4;
            unsigned char r = img->pixels[src];
            unsigned char g = img->pixels[src + 1];
            unsigned char b = img->pixels[src + 2];
            // alpha channel ignored here, but you could blend if desired
            popg_PUTPIXEL(sgfx, startX + x, startY + y, r, g, b);
        }
    }

    sgfx->blit(sgfx);

    nvg_free_image(svc, img);
    sgfx->deinit(sgfx);
    return 0;
}

#define nvg_IMPLEMENTATION
#include "nvgif.h"