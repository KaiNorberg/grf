#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>

#include "../../grf.h"

static int freetype_get_advance(FT_Face face, unsigned int charCode) 
{
    FT_UInt glyphIndex = FT_Get_Char_Index(face, charCode);
    if (!glyphIndex)
    {
        return 0;
    }
    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT))
    {
        return 0;
    }
    return (int)(face->glyph->advance.x >> 6);
}

int ttf_to_grf(const char* ttfPath, const char* grfPath, int size)
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

    error = FT_New_Face(library, ttfPath, 0, &face);
    if (error) 
    {
        fprintf(stderr, "ttf2grf: error loading font '%s': %d\n", ttfPath, error);
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

    hb_font_t *hbFont = hb_ft_font_create(face, NULL);
    if (!hbFont) {
        fprintf(stderr, "ttf2grf: error creating HarfBuzz font\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return -1;
    }
    hb_font_set_scale(hbFont, face->units_per_EM, face->units_per_EM);

    hb_buffer_t *hbBuffer = hb_buffer_create();
    hb_buffer_set_direction(hbBuffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(hbBuffer, HB_SCRIPT_LATIN);
    hb_buffer_set_language(hbBuffer, hb_language_from_string("en", -1));

    FILE* output = fopen(grfPath, "wb");
    if (output == NULL)
    {
        fprintf(stderr, "ttf2grf: error opening '%s'\n", grfPath);
        hb_font_destroy(hbFont);
        hb_buffer_destroy(hbBuffer);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return -1;
    }

    grf_t grf = {0};
    grf.magic = GRF_MAGIC;
    grf.ascender = (int16_t)(face->size->metrics.ascender >> 6);
    grf.descender = (int16_t)(face->size->metrics.descender >> 6);
    grf.height = (int16_t)(face->size->metrics.height >> 6);

    uint8_t* glyphs = NULL;
    uint64_t glyphsSize = 0;

    for (int i = 0; i < 256; i++)
    {
        FT_UInt index = FT_Get_Char_Index(face, i);
        if (index == 0)
        {
            grf.glyphOffsets[i] = GRF_NONE;
            continue;
        }

        error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
        if (error)
        {
            grf.glyphOffsets[i] = GRF_NONE;
            continue;
        }
        
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error)
        {
            grf.glyphOffsets[i] = GRF_NONE;
            continue;
        }

        uint64_t currentGlyphSize = sizeof(grf_glyph_t) + face->glyph->bitmap.width * face->glyph->bitmap.rows * sizeof(uint8_t);

        glyphsSize += currentGlyphSize;
        glyphs = realloc(glyphs, glyphsSize);
        
        grf_glyph_t* glyph = (grf_glyph_t*)((uint8_t*)glyphs + (glyphsSize - currentGlyphSize));
        glyph->bearingX = face->glyph->bitmap_left;
        glyph->bearingY = face->glyph->bitmap_top;
        glyph->advanceX = (int16_t)(face->glyph->advance.x >> 6);
        glyph->advanceY = (int16_t)(face->glyph->advance.y >> 6);
        glyph->width = face->glyph->bitmap.width;
        glyph->height = face->glyph->bitmap.rows;

        memcpy(glyph->buffer, face->glyph->bitmap.buffer, glyph->width * glyph->height * sizeof(uint8_t));
 
        grf.glyphOffsets[i] = (uint32_t)((uint64_t)glyph - (uint64_t)glyphs);
    }

    uint8_t* kernBlocks = NULL;
    uint64_t kernBlocksSize = 0;

    printf("ttf2grf: processing kerning using HarfBuzz...\n");

    for (int firstChar = 0; firstChar < 256; firstChar++)
    {
        FT_UInt firstIndex = FT_Get_Char_Index(face, firstChar);
        if (firstIndex == 0)
        {
            grf.kernOffsets[firstChar] = GRF_NONE;
            continue;
        }

        grf_kern_entry_t entries[256];
        uint16_t entryAmount = 0;

        int firstCharAdvance = freetype_get_advance(face, firstChar);
        if (firstCharAdvance == 0 && firstChar != 0) { 
            grf.kernOffsets[firstChar] = GRF_NONE;
            continue;
        }

        for (int secondChar = 0; secondChar < 256; secondChar++)
        {
            FT_UInt secondIndex = FT_Get_Char_Index(face, secondChar);
            if (secondIndex == 0)
            {                    
                continue;
            }

            int secondCharAdvance = freetype_get_advance(face, secondChar);
            if (secondCharAdvance == 0 && secondChar != 0) 
            { 
                continue;
            }

            hb_buffer_clear_contents(hbBuffer);
            hb_buffer_add_utf32(hbBuffer, (const uint32_t[]){(uint32_t)firstChar, (uint32_t)secondChar}, 2, 0, 2);
            
            hb_shape(hbFont, hbBuffer, NULL, 0);

            unsigned int glyphCount;
            hb_glyph_position_t *glyphPos = hb_buffer_get_glyph_positions(hbBuffer, &glyphCount);

            if (glyphCount == 2)
            {
                int16_t kernX = (int16_t)(glyphPos[1].x_offset >> 6);
                int16_t kernY = (int16_t)(glyphPos[1].y_offset >> 6);
                
                if (kernX != 0 || kernY != 0)
                {
                    entries[entryAmount].secondChar = (uint8_t)secondChar;
                    entries[entryAmount].offsetX = kernX;
                    entries[entryAmount].offsetY = kernY;
                    entryAmount++;
                }
            }
        }

        if (entryAmount > 0)
        {
            uint64_t currentBlockSize = sizeof(grf_kern_block_t) + sizeof(grf_kern_entry_t) * entryAmount;

            kernBlocksSize += currentBlockSize;
            kernBlocks = realloc(kernBlocks, kernBlocksSize);

            grf_kern_block_t* block = (grf_kern_block_t*)(kernBlocks + kernBlocksSize - currentBlockSize);
            block->amount = entryAmount;
            memcpy(block->entries, entries, sizeof(grf_kern_entry_t) * entryAmount);

            grf.kernOffsets[firstChar] = (uint32_t)(glyphsSize + ((uint64_t)block - (uint64_t)kernBlocks));
        }
        else
        {
            grf.kernOffsets[firstChar] = GRF_NONE;
        }
    }

    fwrite(&grf, sizeof(grf), 1, output);
    fwrite(glyphs, glyphsSize, 1, output);
    if (kernBlocks != NULL)
    {
        fwrite(kernBlocks, kernBlocksSize, 1, output);
    }

    free(glyphs);
    free(kernBlocks);
    fclose(output);
    
    hb_buffer_destroy(hbBuffer);
    hb_font_destroy(hbFont);

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