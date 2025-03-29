#pragma once
#undef internal
#include <ft2build.h>
#include FT_FREETYPE_H
#define internal static


global_variable FT_Library library;

struct FontInfo
{
    i32 ascent;
    i32 descent;
    i32 line_height;
    i32 max_glyph_width;
    i32 max_glyph_height;
    i32 max_char_width;
    FontGlyph font_table[300];
};

internal void font_init(Arena *arena);
internal FontInfo font_load();
internal FontGlyph load_font_glyph(FT_Face face, char codepoint, FontInfo *info);