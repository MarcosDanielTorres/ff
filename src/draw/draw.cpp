internal void 
draw_bitmap(OS_PixelBuffer *buffer, i32 x, i32 y, Bitmap bitmap, u32 color) 
{
    //aim_profiler_time_function;
    u32 new_x = x;
    //u32 new_y = y - bitmap_top;
    u32 new_y = y;
    u32 width = bitmap.width;
    u32 height = bitmap.height;

    if (new_x > buffer->width)
    {
        new_x = buffer->width;
    }

    if(new_x + width > buffer->width)
    {

        width = buffer->width - new_x;
    }

    if(new_y > buffer->height)
    {
        new_y = buffer->height;
    }

    if(new_y + height > buffer->height)
    {
        
        height = buffer->height - new_y;
    }

    u8* dest_row = buffer->pixels + new_y * buffer->pitch + new_x * 4;
    u32* dest_row2 = (u32*)buffer->pixels + new_y * buffer->width + new_x;
    {
        for(i32 y = 0; y < height; y++){
            u32* destrow = (u32*) dest_row2;
            u32* dest_pixel = (u32*) dest_row;
            for(i32 x = 0; x < width; x++) {
                u8* src_pixel = bitmap.buffer + width * y + x;
                u32 alpha = *src_pixel << 24;
                u32 red = *src_pixel << 16;
                u32 green = *src_pixel << 8;
                u32 blue = *src_pixel;
                //*dest_pixel++ = color;

                //u32* sourcerow = (u32*)hdp.bitmap.buffer + hdp.bitmap.width * y + x;
                //f32 sa = (f32)((*sourcerow >> 24) & 0xFF) / 255.0f;
                //f32 sr = (f32)((*sourcerow >> 16) & 0xFF);
                //f32 sg = (f32)((*sourcerow >> 8) & 0xFF);
                //f32 sb = (f32)((*sourcerow >> 0) & 0xFF);

                //f32 dr = (f32)((*destrow >> 16) & 0xFF);
                //f32 dg = (f32)((*destrow >> 8) & 0xFF);
                //f32 db = (f32)((*destrow >> 0) & 0xFF);

                //// Blend equation: linear interpolation (lerp)
                //u32 nr = u32((1.0f - sa) * dr + sa*sr);
                //u32 ng = u32((1.0f - sa) * dg + sa*sg);
                //u32 nb = u32((1.0f - sa) * db + sa*sb);

                //*destrow = (nr << 16) | (ng << 8) | (nb << 0);
                //destrow++;


                // alright so dest is the background and in this case the dest_row (buffer->memory)
                // formula: alpha * src + (1 - alpha) * dest
                // => (1.0f - t) * A + t * B
                // => A-tA + tB
                // => A+t(B - A)

                f32 sa = (f32)(*src_pixel / 255.0f);

                u32 color2 = color;

                f32 sr = (f32)((color2 >> 16) & 0xFF);
                f32 sg = (f32)((color2 >> 8) & 0xFF);
                f32 sb = (f32)((color2 >> 0) & 0xFF);

                f32 dr = (f32)((*destrow >> 16) & 0xFF);
                f32 dg = (f32)((*destrow >> 8) & 0xFF);
                f32 db = (f32)((*destrow >> 0) & 0xFF);
                u32 nr = u32((1.0f - sa) * dr + sa*sr);
                u32 ng = u32((1.0f - sa) * dg + sa*sg);
                u32 nb = u32((1.0f - sa) * db + sa*sb);

                *destrow = (nr << 16) | (ng << 8) | (nb << 0);
                //*destrow = red;
                destrow++;

            }
            dest_row += buffer->pitch;
            dest_row2 += buffer->width;
        }

    }
}

internal void
draw_line(OS_PixelBuffer* buffer, i32 x0, i32 y0, i32 x1, i32 y1, u32 color, DrawLineFlags flags)
{
    // only supports movement in one component!
    u8* row = buffer->pixels + buffer->pitch * y0 + x0 * 4;

    if (x0 == x1)
    {
        for(i32 y = 0; y < y1 - y0; y++)
        {
            u32* pixel = (u32*) row;
            *pixel = color;
            row += buffer->pitch;
        }
    }
    else
    {
        if(y0 == y1)
        {
            u32* pixel = (u32*) row;
            for(i32 x = 0; x < x1 - x0; x++)
            {
                *pixel++ = color;
            }

        }
        else
        {
            assert(1 == 2);
        }
    }
}

internal void
draw_rect(OS_PixelBuffer* buffer, i32 dest_x, i32 dest_y, i32 width, i32 height, u32 color, DrawRectFlags flags) 
{
    u8* row = buffer->pixels + buffer->pitch * dest_y + dest_x * 4;

    for(i32 y = 0; y < height; y++) {
        u32* pixel = (u32*)row;
        for(i32 x = 0; x < width; x++) {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

internal void 
clear_buffer(OS_PixelBuffer *buffer, u32 color) 
{

    draw_rect(buffer, 0, 0, buffer->width, buffer->height, color); 
}

internal void
draw_text(OS_PixelBuffer *buffer, i32 x, i32 y, char* text, FontGlyph* font_table, u32 text_color)
{
    u32 line_offset = x;
    for(char* c = text; *c != '\0'; c++)
    {
        // todo see how he does the `DEBUGTextOut`
        // todo see how does he get the correct bitmap font
        FontGlyph* glyph = &font_table[(u32)*c];
        if(*c == '\n')
        {
            y += 20;
        }
        
        // y - glyph->bitmap causes the 0,0 to be at the bot-left corner. opposed as the rect and bitmap which are renderered with 0,0 at the top-left
        draw_bitmap(buffer, glyph->bitmap_left + line_offset, y - glyph->bitmap_top, glyph->bitmap, text_color);
        line_offset += glyph->advance_x >> 6;
        //draw_bitmap(buffer, x, y - bitmap.bitmap_top, c);
    }
}

internal void
draw_text(OS_PixelBuffer *buffer, i32 x, i32 y, Str8 text, FontGlyph* font_table, u32 text_color)
{
    // todo remove this from here put it in the font info
    u32 tab_size = 4;
    u32 line_offset = x;
    for(u32 i = 0; i < text.size; i++)
    {
        // todo see how he does the `DEBUGTextOut`
        // todo see how does he get the correct bitmap font
        if(text.str[i] == '\n')
        {
            y += 20;
            line_offset = x;
        }
        else
        {
            if(text.str[i] == '\t')
            {
                FontGlyph* space_glyph = &font_table[(u32)' '];
                line_offset += (space_glyph->advance_x >> 6) * tab_size;
            }
            else
            {
                FontGlyph* glyph = &font_table[(u32)text.str[i]];
                // y - glyph->bitmap causes the 0,0 to be at the bot-left corner. opposed as the rect and bitmap which are renderered with 0,0 at the top-left
                draw_bitmap(buffer, glyph->bitmap_left + line_offset, y - glyph->bitmap_top, glyph->bitmap, text_color);
                line_offset += glyph->advance_x >> 6;
                //draw_bitmap(buffer, x, y - bitmap.bitmap_top, c);

            }
        }
    }
}