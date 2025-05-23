#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../../grf.h"

int ttf_to_grf(const char* ttf, const char* grf, int size)
{
    FT_Library library;
    FT_Face face;
    FT_Error error;

    error = FT_Init_FreeType(&library);
    if (error) 
    {
        fprintf(stderr, "ttf2grf: error initializing FreeType: %d\n", error);
        return -1;
    }

    error = FT_New_Face(library, ttf, 0, &face);
    if (error) 
    {
        fprintf(stderr, "ttf2grf: error loading font '%s': %d\n", ttf, error);
        FT_Done_FreeType(library);
        return -1;
    }

    error = FT_Set_Pixel_Sizes(face, 0, size);
    if (error) 
    {
        fprintf(stderr, "ttf2grf: error setting font size: %d\n", error);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return -1;
    }

    FILE* output = fopen(grf, "wb");
    if (output == NULL)
    {
        fprintf(stderr, "ttf2grf: error opening '%s'\n", grf);
        return -1;
    }

    grf_header_t header;
    header.magic = GRF_MAGIC;
    // Division by 64 due to FreeTypes pixed point stuff
    header.ascender = (int16_t)(face->size->metrics.ascender / 64);
    header.descender = (int16_t)(face->size->metrics.descender / 64);
    header.height = (int16_t)(face->size->metrics.height / 64);

    uint8_t* glyphs = NULL;
    uint64_t glyphsSize = 0;

    for (int i = 0; i < 256; i++)
    {
        FT_UInt index = FT_Get_Char_Index(face, i);
        if (index == 0)
        {
            header.offsetTable[i] = UINT32_MAX;
            continue;
        }

        error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
        if (error)
        {
            header.offsetTable[i] = UINT32_MAX;
            continue;
        }
        
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error)
        {
            header.offsetTable[i] = UINT32_MAX;
            continue;
        }

        uint64_t currentGlyphSize = sizeof(grf_glyph_t) + face->glyph->bitmap.width * face->glyph->bitmap.rows * sizeof(uint8_t);

        glyphsSize += currentGlyphSize;
        glyphs = realloc(glyphs, glyphsSize);
        
        grf_glyph_t* glyph = (grf_glyph_t*)((uint8_t*)glyphs + (glyphsSize - currentGlyphSize));
        glyph->bearingX = face->glyph->bitmap_left;
        glyph->bearingY = face->glyph->bitmap_top;
        glyph->advanceX = (int16_t)(face->glyph->advance.x / 64);
        glyph->advanceY = (int16_t)(face->glyph->advance.y / 64);
        glyph->width = face->glyph->bitmap.width;
        glyph->height = face->glyph->bitmap.rows;

        memcpy(glyph->buffer, face->glyph->bitmap.buffer, glyph->width * glyph->height * sizeof(uint8_t));
 
        header.offsetTable[i] = (uint32_t)((uint64_t)glyph - (uint64_t)glyphs);
    }

    fwrite(&header, sizeof(header), 1, output);
    fwrite(glyphs, glyphsSize, 1, output);

    free(glyphs);
    fclose(output);
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s <input.ttf> <output.grf> [font_size]\n", argv[0]);
        printf("Example: %s my_ttf.ttf my_grf.grf 16\n", argv[0]);
        return 1;
    }

    const char* ttf = argv[1];
    const char* grf = argv[2];

    int fontSize;
    if (argc >= 4)
    {
        fontSize = atoi(argv[3]);
        if (fontSize < 0)
        {
            fprintf(stderr, "ttf2grf: negative font size specified\n");
            return 0;
        }
    }
    else
    {
        fontSize = 16;
    }

    printf("ttf2grf: parsing ttf file...\n");

    if (ttf_to_grf(ttf, grf, fontSize) == -1)
    {
        fprintf(stderr, "ttf2grf: failed to parse ttf file\n");
        return 0;
    }

    printf("ttf2grf: done!\n");
    return 0;
}