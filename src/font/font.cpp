#include "font/font.h"
/* NOTEs
* - face->glyph->bitmap
* --- face->glyph->bitmap.buffer
* - Global Metrics: face->(ascender, descender, height): For general font information
* - Scaled Global Metrics: face->size.metrics.(ascender, descender, height): For rendering text
* - "The metrics found in face->glyph->metrics are normally expressed in 26.6 pixel format"
    to convert to pixels >> 6 (divide by 64). Applies to scaled metrics as well.
* - Calling `FT_Set_Char_Size` sets `FT_Size` in the active `FT_Face` and is used by functions like: `FT_Load_Glyph`
    https://freetype.org/freetype2/docs/reference/ft2-sizing_and_scaling.html#ft_size
* - face->bitmap_top is the distance from the top of the bitmap to the baseline. y+ positive
    https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html#ft_glyphslot
* - glyph->advance.x must be shifted >> 6 as well if pixels are wanted
*/

internal void 
font_init()
{
    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        const char* err_str = FT_Error_String(error);
        printf("FT_Init_FreeType: %s\n", err_str);
        exit(1);
    }
}

internal FontInfo
font_load(Arena* arena)
{
    FontInfo info = {0};
    OS_FileReadResult font_file = os_file_read(arena, "C:\\Windows\\Fonts\\CascadiaMono.ttf");

    FT_Face face = {0};
    if(font_file.data)
    {
        FT_Open_Args args = {0};
        args.flags = FT_OPEN_MEMORY;
        args.memory_base = (u8*) font_file.data;
        args.memory_size = font_file.size;

        FT_Error opened_face = FT_Open_Face(library, &args, 0, &face);
        if (opened_face) 
        {
            const char* err_str = FT_Error_String(opened_face);
            printf("FT_Open_Face: %s\n", err_str);
            exit(1);
        }

        FT_Error set_char_size_err = FT_Set_Char_Size(face, 4 * 64, 4 * 64, 300, 300);
        info.ascent = face->size->metrics.ascender >> 6;    // Distance from baseline to top (positive)
        info.descent = - (face->size->metrics.descender >> 6); // Distance from baseline to bottom (positive)
        info.line_height = face->size->metrics.height >> 6;

        {
            //aim_profiler_time_block("font_table creation")
            for(u32 codepoint = '!'; codepoint <= '~'; codepoint++) {
                //font_table[u32(codepoint)] = load_glyph(face, char(codepoint));
                info.font_table[u32(codepoint)] = load_font_glyph(face, char(codepoint), &info);
            }
            info.font_table[u32(' ')] = load_font_glyph(face, char(' '), &info);
        }
    }
    return info;
}

FontGlyph load_font_glyph(FT_Face face, char codepoint, FontInfo *info) {
    FontGlyph result = {0};
    // The index has nothing to do with the codepoint
    u32 glyph_index = FT_Get_Char_Index(face, codepoint);
    if (glyph_index) 
    {
        FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT); 
        if (load_glyph_err) 
        {
            const char* err_str = FT_Error_String(load_glyph_err);
            printf("FT_Load_Glyph: %s\n", err_str);
        }

        FT_Error render_glyph_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (render_glyph_err) 
        {
            const char* err_str = FT_Error_String(render_glyph_err);
            printf("FT_Load_Glyph: %s\n", err_str);
        }

		info->max_glyph_width = max(info->max_glyph_width, face->glyph->bitmap.width);
		info->max_glyph_height = max(info->max_glyph_height, face->glyph->bitmap.rows);

        result.bitmap.width = face->glyph->bitmap.width;
        result.bitmap.height = face->glyph->bitmap.rows;
        result.bitmap.pitch = face->glyph->bitmap.pitch;
        
        result.bitmap_top = face->glyph->bitmap_top;
        result.bitmap_left = face->glyph->bitmap_left;
        result.advance_x = face->glyph->advance.x;
        info->max_char_width = max(info->max_char_width, result.advance_x);

        FT_Bitmap* bitmap = &face->glyph->bitmap;
        result.bitmap.buffer = (u8*)malloc(result.bitmap.pitch * result.bitmap.height);
        memcpy(result.bitmap.buffer, bitmap->buffer, result.bitmap.pitch * result.bitmap.height);
    }
    return result;
}
