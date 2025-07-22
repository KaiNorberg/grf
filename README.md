# Grayscale Raster Font or .grf ![](https://img.shields.io/badge/License-MIT-green)

The Grayscale Raster Font format is designed to bring good-*ish* looking font rendering to hobbyist operating system developers or other environments where using modern font formats like .ttf is undesirable.

## Why does this exist?

Modern font formats like .ttf are incredibly complicated and out of reach to most hobbyists without the use of some library like FreeType, which many might not want to use for the sake of making everything "from scratch" and even if thats not a concern porting FreeType is not a cakewalk. This leaves the easy option of using bitmap fonts, for example PC Screen Fonts (.psf), which are simply ugly and lack any form of antialiasing, kerning or even varying advance steps. The .grf format attempts to solve these issues by providing a easy to use middle ground between bitmap fonts and modern font formats.

## What is it?

A .grf file supports anti-ailiasing, kerning and other alignment variables, but it is rasterized meaning it stores raw pixel data instead of using curves to define its glyphs like modern fonts do. This allows us to skip the most complicated part of modern fonts, actually rasterizing it, this is not unusual as this is how basic bitmap fonts already work, however most bitmap fonts only use 1 bit to store their pixel data leading to highly pixelated output. GRF on the other hand use 1 full byte (8 bit grayscale) for each pixel where 0x00 means "fully transperant" and 0xFF means "fully solid". The .grf format also attempts to minimize the amount of postprocessing required after loading a font, with the goal of being as simple as possible balanced with the desire for good-*ish* looking fonts.

## Limitations

The Grayscale Raster font format is by no means "better" then a modern font. The biggest limitation is that each .grf file supports one and only one font size, this means you'll need to generate a separate .grf file for each desired font, size, and style (e.g., bold, italic) combination. It also only supports ASCII as Unicode support would result in massive file sizes, consider that we are using 1 byte per pixel for example a font size of 8x16 would result in 128 bytes per glyph for just the pixels, then consider that kerning data scales exponentially since kerning is done for each pair of glyphs, and you see how file sizes for Unicode with its thousands of glyphs would be unreasonable. Perhaps in the future a version of this format with file compression would be able to reasonably handle Unicode, but that would require testing. There are also some additional font rendering features that are unavailable, but it produces a rather convincing end result. So if you came here for sub-pixel perfect font rendering, then you are in the wrong place. But if pixel perfect is good enough, then you might be in the right place.

## Tools

Below is a list of the currently available tools for .grf.

### font2grf

Can be used to convert modern font formats to .grf, for example .ttf to .grf.

The tool can be used with the following steps.

1. Clone (download) this repository, you can use the Code button at the top left of the screen, or if you have git installed use the following command `git clone --recursive https://github.com/KaiNorberg/grf`.
2. Move into the grf/tools/font2grf directory.
3. Run the `make` command.
4. Use the generated `font2grf` executable following this example `./font2grf <input.ttf> <output.grf> [font_size]`

Done!

## Screenshots

![Desktop Screenshot from PatchworkOS](meta/screenshots/desktop.png)

## Implementations

* [PatchworkOS](https://github.com/KaiNorberg/PatchworkOS) Hobbyist OS

## Format

The following describes the .grf format. Note that all multi-byte fields are stored in little-endian byte order. You can also look inside the [grf.h](grf.h) file if you want everything as C structures.

### `grf_t` (File Header)

This structure is a fixed-size header located at the very beginning of the `.grf` file. It contains global font metrics and offset tables to variable-length glyph and kerning data.

| Offset | Type                     | Description                                                                                                                                                             |
| :----- | :----------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0      | `uint32_t`               | Magic value, should be equal to GRF0 in ascii or 0x47524630, last byte acts as version number.                                                                          |
| 4      | `int16_t`                | Font ascender in pixels.                                                                                                                                                |
| 6      | `int16_t`                | Font descender in pixels.                                                                                                                                               |
| 8      | `int16_t`                | Total line height in pixels.                                                                                                                                            |
| 10     | `uint32_t[256]`          | Offsets to each `grf_glyph_t` in `grf_t::buffer`, indexed by ascii chars. `GRF_NONE` means "none".                                                                      |
| 1034   | `uint32_t[256]`          | Offsets to each `grf_kern_block_t` in `grf_t::buffer`, indexed by the starting ascii char. `GRF_NONE` means "none".                                                     |
| 2058   | `uint8_t[]`              | Glyphs and kerning info is stored here. No guarantee of glyph or kerning orders, could be one after the other, interleaved, etc, always use the offsets.                |

Note that the value of `GRF_NONE` is `UINT32_MAX` and that a offset of 0 is valid.

### `grf_glyph_t` (Glyph Structure)

This structure defines the bitmap data and metrics for a single character (glyph).

| Offset | Type       | Description                                  |
| :----- | :--------- | :------------------------------------------- |
| 0      | `int16_t`  | Horizontal bearing.                          |
| 2      | `int16_t`  | Vertical bearing.                            |
| 4      | `int16_t`  | Horizontal advance.                          |
| 6      | `int16_t`  | Vertical advance, usually 0.                 |
| 8      | `uint16_t` | The width of the buffer in pixels/bytes.     |
| 10     | `uint16_t` | The height of the buffer in pixels/bytes.    |
| 12     | `uint8_t[]`| The pixel buffer, each pixel is 1 byte.      |

### `grf_kern_entry_t` (Kerning Entry)

This structure describes a single kerning adjustment for a pair of characters.

| Offset | Type      | Description                                                    |
| :----- | :-------- | :------------------------------------------------------------- |
| 0      | `uint8_t` | The second character in the kerning pair                       |
| 1      | `int16_t` | The horizontal offset to be added to `advanceX` for this character pair. |
| 3      | `int16_t` | The vertical to be added to `advanceY` for this character pair, probably 0. |

### `grf_kern_block_t` (Kerning Block)

This structure contains a list of kerning entries for a specific starting character.

| Offset | Type             | Description                                                                                                                                                                                                                                                                                                                                                                     |
| :----- | :--------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0      | `uint16_t`       | The amount of kerning entries for this char                                                                                                                                                                                                                                                                                                                             |
| 2      | `grf_kern_entry_t[]` | The entries, these entries will always be sorted by char, so the entry for 'A' is before 'B', this allows for efficient look ups using binary search. |

## Advice for using this format

If you are planning to use this font format I highly recommend using a font with good kerning, most modern font renderers do a lot of fancy stuff to improve the kerning of fonts, all that fancy stuff is basically impossible with this format so we have to solely rely on the kernel in the font itself, a example of a font with good kerning is [Lato](https://fonts.google.com/specimen/Lato).

## Example in C

The following is a example of how you could use this image format in C. Note the use of the [grf.h](grf.h) file.

```c
#include "grf.h"

#include <stdio.h>
#include <stdint.h>

grf_t* grf_load(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    grf_t* grf = malloc(fileSize);
    if (grf == NULL)
    {
        fclose(file);
        return NULL;
    }

    if (fread(grf, fileSize, 1, file) != 1)
    {
        free(grf);
        fclose(file);
        return NULL;
    }

    fclose(file);

    if (grf->magic != GRF_MAGIC)
    {
        free(grf);
        return NULL;
    }

    // For a fully robust loader, you'd iterate through glyphOffsets and kernOffsets
    // to ensure they point within the allocated fileSize and to valid structure sizes.

    return grf;
}

// Helper function to get kerning offset.
// Note that this could be optimized using binary search since the array is sorted.
int16_t grf_get_kerning_offset(const grf_t* grf, uint8_t firstChar, uint8_t secondChar)
{
    uint32_t offset = grf->kernOffsets[firstChar];
    if (offset == GRF_NONE)
    {
        return 0;
    }

    grf_kern_block_t* block = (grf_kern_block_t*)(&grf->buffer[offset]);
    for (uint16_t i = 0; i < block->amount; i++)
    {
        if (block->entries[i].secondChar == (uint8_t)secondChar)
        {
            return block->entries[i].offsetX;
        }
        // Becouse the entires are sorted, if we've passed the secondChar, then its not in the entries.
        if (block->entries[i].secondChar > (uint8_t)secondChar)
        {
            break;
        }
    }
    return 0;
}

// The draw functions assume that we are using 32 bit ARGB. 

void grf_draw_char(grf_t* grf, uint32_t* pixels, uint64_t pixelsWidth, uint64_t pixelsHeight, 
    uint64_t pixelsStride, uint64_t xPos, uint64_t yPos, char chr)
{
    uint32_t offset = grf->glyphOffsets[chr];
    if (offset == GRF_NONE) // Glyph does not exist
    {
        return;
    }
    grf_glyph_t* glyph = (grf_glyph_t*)(&grf->buffer[offset]);

    int32_t baselineY = yPos + grf->ascender;

    for (uint16_t y = 0; y < glyph->height; y++)
    {
        for (uint16_t x = 0; x < glyph->width; x++)
        {
            uint8_t gray = glyph->buffer[y * glyph->width + x];

            if (gray > 0)
            {
                int32_t targetX = xPos + glyph->bearingX + x;
                int32_t targetY = baselineY - glyph->bearingY + y;

                if (targetX < 0 || targetY < 0 || targetX >=pixelsWidth ||
                    targetY >= pixelsHeight)
                {
                    continue;
                }

                // The "gray" value is a transperancy value so we we blend the current value in the pixels buffer
                // and a pure white pixel based of the gray value.

                uint32_t pixel = pixels[targetY * pixelsStride + targetX];
                
                // Change this to change the color of the text
                uint32_t srcAlpha = gray;
                uint32_t srcR = 0xFF;
                uint32_t srcG = 0xFF;
                uint32_t srcB = 0xFF;

                uint32_t destR = (pixel >> 16) & 0xFF;
                uint32_t destG = (pixel >> 8) & 0xFF;
                uint32_t destB = (pixel >> 0) & 0xFF;

                uint32_t outR = (srcR * srcAlpha + destR * (255 - srcAlpha)) / 255;
                uint32_t outG = (srcG * srcAlpha + destG * (255 - srcAlpha)) / 255;
                uint32_t outB = (srcB * srcAlpha + destB * (255 - srcAlpha)) / 255;

                pixels[targetY * pixelsStride + targetX] = (0xFF << 24) | (outR << 16) | (outG << 8) | outB;
            }
        }
    }
}

void grf_draw_string(grf_t* grf, uint32_t* pixels, uint64_t pixelsWidth, uint64_t pixelsHeight,
    uint64_t pixelsStride, uint64_t xPos, uint64_t yPos, const char* string, uint64_t length)
{
    int32_t currentX = xPos;
    int32_t currentY = yPos;

    for (uint64_t i = 0; i < length; i++)
    {
        uint32_t offset = grf->glyphOffsets[(uint8_t)string[i]];
        if (offset == GRF_NONE)  // Glyph does not exist
        {
            continue;
        }
        grf_glyph_t* glyph = (grf_glyph_t*)(&grf->buffer[offset]);

        grf_draw_char(grf, pixels, pixelsWidth, pixelsHeight, pixelsStride, currentX, currentY, (uint8_t)string[i]);

        currentX += glyph->advanceX;
        // Apply kerning if the char is not the last char.
        if (i < length - 1)
        {
            currentX += grf_get_kerning_offset(grf, (uint8_t)string[i], (uint8_t)string[i + 1]);
        }
    }
}
```

## Contributing

If you end up creating more tools/plugins or anything else related to the .grf file format feel free to submit a pull request to have it added to this repository.
