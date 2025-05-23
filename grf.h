#ifndef GRF_H
#define GRF_H

#include <stdint.h> 

// This file contains definitions for the grf font format, a font format originally made for PatchworkOS.
// Source: https://github.com/KaiNorberg/grf

#if defined(_MSC_VER)
#define GRF_PACK_START __pragma(pack(push, 1))
#define GRF_PACK_END __pragma(pack(pop))
#define GRF_PACK_STRUCT
#elif defined(__GNUC__) || defined(__clang__)
#define GRF_PACK_START
#define GRF_PACK_END
#define GRF_PACK_STRUCT __attribute__((packed))
#else
#warning "Unsupported compiler: Structures might not be packed correctly."
#define GRF_PACK_START
#define GRF_PACK_END
#define GRF_PACK_STRUCT
#endif

#define GRF_MAGIC 0x47524630 // ASCII for "GRF0"

GRF_PACK_START
typedef struct GRF_PACK_STRUCT
{
    uint32_t magic;           // GRF0
    int16_t ascender;         // Font ascender in pixels
    int16_t descender;        // Font descender in pixels
    int16_t height;       // Total line height in pixels
    uint32_t offsetTable[256]; // Offsets to each glyph relative to the end of the header, indexed by ascii chars. UINT32_MAX means "none".
    // Glyphs are here
} grf_header_t;

typedef struct GRF_PACK_STRUCT
{
    int16_t bearingX;         // Horizontal bearing
    int16_t bearingY;         // Vertical bearing
    int16_t advanceX;         // Horizontal advance
    int16_t advanceY;         // Vertical advance, usually 0
    uint16_t width;           // The width of the buffer in pixels/bytes
    uint16_t height;          // The height of the buffer in pixels/bytes
    uint8_t buffer[];         // The pixel buffer, each pixel is 1 byte
} grf_glyph_t;
GRF_PACK_END

#endif // GRF_H