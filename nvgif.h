#ifndef nvg_IMPLEMENTATION
    #include "popcorn.h"
    #define nvg_COMPRESSION_NONE 0
    #define nvg_COMPRESSION_RLE  1
    
    #define nvg_ERRNO_IO_ERROR 0
    #define nvg_ERRNO_NO_MEMORY 1
    #define nvg_ERRNO_UNSUPPORTED 2
    #define nvg_ERRNO_INVALID_DATA 3
    
    int nvg_errnum = -1;
    char nvg_errval[128];
    
    const char* nvg_errnum_str[4] = {
        "I/O error",
        "not enough memory",
        "unsupported operation",
        "invalid data"
    };
    
    typedef struct {
        unsigned char* pixels; // RGBA buffer
        int width;
        int height;
    } nvg_Image;
    
    // Function declarations
    nvg_Image* nvg_decode_image(pop_Services* svc, const CHAR16* path);
    void nvg_free_image(pop_Services* svc, nvg_Image* img);
#else
    static void nvg__set_error(int code, const char* msg) {
        nvg_errnum = code;
        // naive copy into nvg_errval (no snprintf)
        int j = 0;
        while (msg[j] && j < sizeof(nvg_errval)-1) {
            nvg_errval[j] = msg[j];
            j++;
        }
        nvg_errval[j] = '\0';
    }
    
    /* RLE decode using Popcorn services */
    static unsigned char* nvg__decode_rle(pop_Services* svc,
                                          const unsigned char* row,
                                          int bpp,
                                          int expectedPixels) {
        unsigned char* result = svc->memalloc(svc, expectedPixels*  bpp);
        if (!result) {
            nvg__set_error(nvg_ERRNO_NO_MEMORY, "not enough memory for RLE decode");
            return NULL;
        }
    
        int i = 0, pixelsDecoded = 0, out = 0;
        while (pixelsDecoded < expectedPixels) {
            unsigned char count = row[i];
            const unsigned char* unit = row + i + 1;
            for (int j = 0; j < count && pixelsDecoded < expectedPixels; j++) {
                for (int k = 0; k < bpp; k++) {
                    result[out + k] = unit[k];
                }
                out += bpp;
                pixelsDecoded++;
            }
            i += 1 + bpp;
        }
        return result;
    }
    
    /* Row decode using Popcorn services */
    static unsigned char* nvg__decode_row(pop_Services* svc,
                                          const unsigned char* row,
                                          int comp,
                                          int bpp,
                                          int w) {
        if (comp == nvg_COMPRESSION_NONE) {
            unsigned char* copy = svc->memalloc(svc, w*bpp);
            if (!copy) {
                nvg__set_error(nvg_ERRNO_NO_MEMORY, "not enough memory for raw row");
                return NULL;
            }
            for (int j = 0; j < w*bpp; j++) {
                copy[j] = row[j];
            }
            return copy;
        } else if (comp == nvg_COMPRESSION_RLE) {
            return nvg__decode_rle(svc, row, bpp, w);
        } else {
            nvg__set_error(nvg_ERRNO_UNSUPPORTED, "unsupported compression type");
            return NULL;
        }
    }
    
    nvg_Image* nvg_decode_image(pop_Services* svc, const CHAR16* path) {
        popf_FileMode mode = { .read = TRUE, .bytes = TRUE };
        popf_File* f = svc->fileopen(svc, path, mode);
        if (!f) {
            svc->print(svc, L"nvgif: ");
            svc->perrno(svc, svc->errcode);
            nvg__set_error(nvg_ERRNO_IO_ERROR, "unable to open file");
            return NULL;
        }
    
        unsigned char* buf = f->read(f); // full file contents
        f->close(f);
        if (!buf) {
            svc->errcode = pop_EEOF;
            nvg__set_error(nvg_ERRNO_IO_ERROR, "failed to read file");
            return NULL;
        }
    
        int i = 0;
        if (buf[i++] != 'N' || buf[i++] != 'V' || buf[i++] != 'G') {
            svc->memfree(svc, buf);
            nvg__set_error(nvg_ERRNO_INVALID_DATA, "could not find NVGIF magic");
            return NULL;
        }
    
        int version = buf[i++];
        if (version < 1 || version > 3) {
            svc->memfree(svc, buf);
            nvg__set_error(nvg_ERRNO_UNSUPPORTED, "unsupported NVGIF version (v1-v3 supported)");
            return NULL;
        }
    
        int comp  = (version >= 2) ? buf[i++] : nvg_COMPRESSION_NONE;
        int alpha = (version >= 3) ? buf[i++] : 0;
        int bpp   = alpha ? 4 : 3;
    
        unsigned short w = (buf[i++] << 8) | buf[i++];
        unsigned short h = (buf[i++] << 8) | buf[i++];
    
        unsigned char* pixels = svc->memalloc(svc, w * h * 4);
        if (!pixels) {
            svc->memfree(svc, buf);
            nvg__set_error(nvg_ERRNO_NO_MEMORY, "not enough memory to allocate pixels");
            return NULL;
        }
    
        for (int y = 0; y < h; y++) {
            unsigned short rowlen = (buf[i++] << 8) | buf[i++];
            unsigned char* row = buf + i;
            i += rowlen;
    
            unsigned char* decoded = nvg__decode_row(svc, row, comp, bpp, w);
            if (!decoded) {
                svc->memfree(svc, buf);
                svc->memfree(svc, pixels);
                // nvg__decode_row already sets its own error
                return NULL;
            }
    
            for (int x = 0; x < w; x++) {
                int src = x*  bpp;
                int dst = (y*  w + x)*  4;
                pixels[dst]     = decoded[src];
                pixels[dst + 1] = decoded[src + 1];
                pixels[dst + 2] = decoded[src + 2];
                pixels[dst + 3] = (bpp == 4) ? decoded[src + 3] : 255;
            }
    
            svc->memfree(svc, decoded);
        }
    
        svc->memfree(svc, buf);
    
        nvg_Image* img = svc->memalloc(svc, sizeof(nvg_Image));
        if (!img) {
            svc->memfree(svc, pixels);
            nvg__set_error(nvg_ERRNO_NO_MEMORY, "not enough memory for image struct");
            return NULL;
        }
        img->width  = w;
        img->height = h;
        img->pixels = pixels;
    
        return img;
    }
    
    void nvg_free_image(pop_Services* svc, nvg_Image* img) {
        if (img) {
            svc->memfree(svc, img->pixels);
            svc->memfree(svc, img);
        }
    }
#endif