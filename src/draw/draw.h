struct Bitmap {
    i32 width;
    i32 height;
    i32 pitch;
    u8* buffer;
};

// TODO fix this FontGlyph structure here. Because for font.h to use it it needs first to import this draw.h file
struct FontGlyph {
    Bitmap bitmap;
    i32 bitmap_top;
    i32 bitmap_left;
    i32 advance_x;
};

enum DrawRectFlags
{
    DrawRectFlags_Solid = 1 << 0,
    DrawRectFlags_Outline = 1 << 1,
};
typedef u32 DrawRect_Flags;

enum DrawLineFlags
{
    DrawLineFlags_Solid = 1 << 0,
};
typedef u32 DrawRect_Flags;

struct Point2D
{
    f32 x;
    f32 y;
};

struct Rect2D
{
    f32 x;
    f32 y;
    f32 w;
    f32 h;
};

internal void draw_bitmap(OS_PixelBuffer *buffer, i32 x, i32 y, Bitmap bitmap, u32 color = 0xFFFFFFFF);
internal void draw_line(OS_PixelBuffer* buffer, i32 dest_x, i32 dest_y, i32 width, i32 height, u32 color = 0xFFFFFFFF, DrawLineFlags flags = DrawLineFlags_Solid);
internal void draw_rect(OS_PixelBuffer* buffer, i32 dest_x, i32 dest_y, i32 width, i32 height, u32 color = 0xFFFFFFFF, DrawRectFlags flags = DrawRectFlags_Solid);
internal void draw_rect(OS_PixelBuffer* buffer, Rect2D r, u32 color = 0xFFFFFFFF, DrawRectFlags flags = DrawRectFlags_Solid); 
internal void clear_buffer(OS_PixelBuffer *buffer, u32 color);
internal void draw_text(OS_PixelBuffer* buffer, i32 x, i32 y, const char* text, FontGlyph* font_table, u32 text_color = 0xFFFFFFFF);
internal void draw_text(OS_PixelBuffer* buffer, i32 x, i32 y, Str8 text, FontGlyph* font_table, u32 text_color = 0xFFFFFFFF);