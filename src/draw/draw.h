struct Bitmap {
    i32 width;
    i32 height;
    i32 pitch;
    u8* buffer;
};

struct FontGlyph {
    Bitmap bitmap;
    i32 bitmap_top;
    i32 bitmap_left;
    i32 advance_x;
};

internal void draw_bitmap(OS_PixelBuffer *buffer, i32 x, i32 y, Bitmap bitmap);
internal void draw_rect(OS_PixelBuffer* buffer, i32 dest_x, i32 dest_y, i32 width, i32 height, u32 color);
internal void clear_buffer(OS_PixelBuffer *buffer, u32 color);
internal void draw_text(OS_PixelBuffer* buffer, i32 x, i32 y, char* text, FontGlyph* font_table);