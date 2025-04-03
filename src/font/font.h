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
    // probably renamed to max_char_advancement (same in monospace)
    i32 max_char_width;
    FontGlyph font_table[300];
};

// TODO maybe this could be transformed into a Dim2D
struct TextSize
{
    f32 w;
    f32 h;
    u32 lines;
};

internal void font_init(Arena *arena);
internal FontInfo font_load();
internal FontGlyph font_load_glyph(FT_Face face, char codepoint, FontInfo *info);
internal TextSize font_get_text_size(FontInfo *text_info, Str8 text);