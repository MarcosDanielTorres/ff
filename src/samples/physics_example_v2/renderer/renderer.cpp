
// Vertex ordering
// x+ right, y+ up, z- into the screen

void check_bounds(RenderGroup *render_group, u32 vertex_count, u32 index_count)
{
    AssertGui(vertex_count < render_group->max_vertex_count, "max vertex count exceeded! %d\n", vertex_count);
    AssertGui(index_count < render_group->max_index_count, "max index count exceeded! %d\n", index_count);
}

#if 0
void push_triangle(RenderGroup *render_group, Vec3 tri_points[3], u16 tri_indices[3])
{
    check_bounds(render_group, 3 + render_group->vertex_count, 3 + render_group->index_count);

    TextureQuadVertex *v = render_group->vertex_array + render_group->vertex_count;
    glm::vec2 uv0 = glm::vec2(0.0f, 0.0f);
    glm::vec2 uv1 = glm::vec2(0.0f, 0.0f);
    glm::vec2 uv2 = glm::vec2(0.0f, 0.0f);
    glm::vec2 uv3 = glm::vec2(0.0f, 0.0f);
    glm::vec4 c = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    v[0] = {tri_points[0], uv0, c};
    v[1] = {tri_points[1], uv1, c};
    v[2] = {tri_points[2], uv2, c};

    u16 *idx = render_group->index_array + render_group->index_count;
    u16 offset = render_group->vertex_count;
    idx[0] = offset + 0;
    idx[1] = offset + 1;
    idx[2] = offset + 2;

    render_group->vertex_count += 3;
    render_group->index_count += 3;

}
#endif


internal void
push_rect(RenderGroup *render_group, Vec2 quad_points[4], f32 border_radius = 0.0f, f32 border_thickness = 0.0f, f32 omit_texture = 0.0f,
    glm::vec4 c = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    glm::vec2 uv0 = glm::vec2(0, 0),
    glm::vec2 uv1 = glm::vec2(1, 0),
    glm::vec2 uv2 = glm::vec2(1, 1),
    glm::vec2 uv3 = glm::vec2(0, 1))
{
    check_bounds(render_group, 4 + render_group->vertex_count, 6 + render_group->index_count);

    TextureQuadVertex *v = render_group->vertex_array + render_group->vertex_count;
    v[0] = {quad_points[0], uv0, c, omit_texture, border_radius, border_thickness,};
    v[1] = {quad_points[1], uv1, c, omit_texture, border_radius, border_thickness,};
    v[2] = {quad_points[2], uv2, c, omit_texture, border_radius, border_thickness,};
    v[3] = {quad_points[3], uv3, c, omit_texture, border_radius, border_thickness,};

    u16 *idx = render_group->index_array + render_group->index_count;
    u16 offset = render_group->vertex_count;
    idx[0] = offset + 0;
    idx[1] = offset + 1;
    idx[2] = offset + 2;
    idx[3] = offset + 2;
    idx[4] = offset + 3;
    idx[5] = offset + 0;

    render_group->vertex_count += 4;
    render_group->index_count += 6;
}

internal void
push_rect(RenderGroup *render_group, f32 x, f32 y, f32 w, f32 h, glm::vec4 c = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Vec2 quad_points[4] = 
    {
        {x, y},
        {x + w, y},
        {x + w, y - h},
        {x, y - h}
    };
    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);
    push_rect(render_group, quad_points, 0.0f, 0.0f, 1.0f, c, uv0, uv1, uv2, uv3);
}

#if 0
internal void
push_rect_border(RenderGroup *render_group, f32 x, f32 y, f32 w, f32 h, f32 border_radius = 0.0f, f32 border_thickness = 0.0f, glm::vec4 color = glm::vec4(1.0f))
{
    const glm::vec3 quad_points[4] = 
    {
        glm::vec3(x, y, 0),
        glm::vec3(x + w, y, 0),
        glm::vec3(x + w, y - h, 0),
        glm::vec3(x, y - h, 0),
    };
    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);
    push_rect(render_group, quad_points, border_radius, border_thickness, 1.0f, color, uv0, uv1, uv2, uv3);
}
#endif

internal void
push_glyph(RenderGroup *render_group, FontGlyph *glyph, f32 x, f32 baseline, glm::vec4 color)
{
    // TODO renombrar!
    i32 glyph_width = glyph->bitmap.width;
    i32 glyph_height = glyph->bitmap.height;
    i32 glyph_top = glyph->bitmap_top;
    i32 glyph_left = glyph->bitmap_left;


    f32 glyph_uv0_x = glyph->uv0_x;
    f32 glyph_uv1_x = glyph->uv1_x;
    f32 glyph_uv2_x = glyph->uv2_x;
    f32 glyph_uv3_x = glyph->uv3_x;

    f32 glyph_uv0_y = glyph->uv0_y;
    f32 glyph_uv1_y = glyph->uv1_y;
    f32 glyph_uv2_y = glyph->uv2_y;
    f32 glyph_uv3_y = glyph->uv3_y;


    glm::vec2 uv0 = glm::vec2(glyph_uv0_x, glyph_uv0_y);
    glm::vec2 uv1 = glm::vec2(glyph_uv1_x, glyph_uv1_y);
    glm::vec2 uv2 = glm::vec2(glyph_uv2_x, glyph_uv2_y);
    glm::vec2 uv3 = glm::vec2(glyph_uv3_x, glyph_uv3_y);
    /* NOTE 
        descent = glyph_height - glyph_top
        ascent = glyph_top
        glyph_height = descent + glyph_top
    */ 
    Vec2 quad_points[4] = 
    {
        {x + glyph_left, baseline + (glyph_height - glyph_top)},
        {x + glyph_left + glyph_width, baseline + (glyph_height - glyph_top)},
        {x + glyph_left + glyph_width,  baseline - glyph_top},
        {x + glyph_left, baseline - glyph_top}
    };
    
    push_rect(render_group, quad_points, 0.0f, 0.0f, 0.0f, color, uv0, uv1, uv2, uv3);
}


internal void
push_circle(RenderGroup *render_group, Vec2 center, f32 radius = 1.0f, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Vec2 quad_points[4] = 
    {
        {center.x - radius, center.y + radius},
        {center.x + radius, center.y + radius},
        {center.x + radius, center.y - radius},
        {center.x - radius, center.y - radius}
    };
    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);
    push_rect(render_group, quad_points, 0.5f, 0.0f, 1.0f, color, uv0, uv1, uv2, uv3);

}

internal void
push_circle(RenderGroup *render_group, f32 center_x, f32 center_y, f32 radius = 1.0f, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Vec2 center = {center_x, center_y};
    push_circle(render_group, center, radius, color);
}


internal void
push_line_2d(RenderGroup *render_group, Vec2 from, Vec2 to, f32 thickness = 0.05f, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Vec2 dir = to - from;
    dir = {-dir.y, dir.x};
    Vec2 slope = norm(dir);
    
    Vec2 halfoffset = 0.5f * thickness * slope;

    glm::vec2 UV0 = glm::vec2{0, 0};
    glm::vec2 UV1 = glm::vec2{1, 0};
    glm::vec2 UV2 = glm::vec2{1, 1};
    glm::vec2 UV3 = glm::vec2{0, 1};

    Vec2 P0 = {from + halfoffset};
    Vec2 P1 = {to + halfoffset};
    Vec2 P2 = {to - halfoffset};
    Vec2 P3 = {from - halfoffset};

    Vec2 quad_points[4] = 
    {
        P0, P1, P2, P3
    };
    push_rect(render_group, quad_points, 0.0f, 0.0f, 1.0f, color, UV0, UV1, UV2, UV3);
}

internal void
push_line_2d(RenderGroup *render_group, f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness = 0.05f, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    Vec2 from = {x0, y0};
    Vec2 to = {x1, y1};
    push_line_2d(render_group, from, to, thickness, color);
}

#if 0
internal void
push_line(RenderGroup *render_group, glm::vec3 from, glm::vec3 to, f32 thickness = 0.05f, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    glm::vec3 line = to - from;
    glm::vec3 dir = glm::normalize(line);

    // Pick a stable world-space up vector that's NOT parallel to the line
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (fabs(glm::dot(dir, up)) > 0.99f) {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    // Compute a perpendicular direction to make the quad thickness
    glm::vec3 thicknessdir = glm::normalize(glm::cross(dir, up));
    glm::vec3 halfoffset = 0.5f * thickness * thicknessdir;

    glm::vec2 UV0 = glm::vec2{0, 0};
    glm::vec2 UV1 = glm::vec2{1, 0};
    glm::vec2 UV2 = glm::vec2{1, 1};
    glm::vec2 UV3 = glm::vec2{0, 1};

    glm::vec3 P0 = from + halfoffset;
    glm::vec3 P1 = to   + halfoffset;
    glm::vec3 P2 = to   - halfoffset;
    glm::vec3 P3 = from - halfoffset;

    Vec3 quad_points[4] = 
    {
        P0, P1, P2, P3
    };
    push_rect(render_group, quad_points, 0.0f, 0.0f, 1.0f, color, UV0, UV1, UV2, UV3);
}
#endif

internal void
push_text(RenderGroup *render_group, FontInfo *font_info, Str8 text, f32 x, f32 baseline, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    f32 pen_x = x;
    f32 pen_y = baseline;
    FontGlyph *font_table = font_info->font_table;

    for(u32 i = 0; i < text.size; i++)
    {
        char c = text.str[i];
        u32 codepoint = u32(c);
        FontGlyph *glyph = &font_table[codepoint];
        push_glyph(render_group, glyph, pen_x, pen_y, color);
        pen_x += glyph->advance_x >> 6;
    }
}

internal void
push_text(RenderGroup *render_group, FontInfo *font_info, char *text, f32 x, f32 baseline, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
{
    f32 pen_x = x;
    f32 pen_y = baseline;
    FontGlyph *font_table = font_info->font_table;

    for(char *c = text; *c != '\0'; c++)
    {
        u32 codepoint = u32(*c);
        FontGlyph *glyph = &font_table[codepoint];
        push_glyph(render_group, glyph, pen_x, pen_y, color);
        pen_x += glyph->advance_x >> 6;

    }
}

#if 0
// NOTE Shouldnt even use this one its calculating the position inside of the function. which is not right
void push_quad(RenderGroup *render_group, glm::vec3 center, glm::vec2 radius, glm::vec4 c)
{
    check_bounds(render_group, 4 + render_group->vertex_count, 6 + render_group->index_count);

    glm::vec3 p0 = center + glm::vec3(-radius.x, -radius.y, 0.0f);
    glm::vec3 p1 = center + glm::vec3(+radius.x, -radius.y, 0.0f);
    glm::vec3 p2 = center + glm::vec3(+radius.x, +radius.y, 0.0f);
    glm::vec3 p3 = center + glm::vec3(-radius.x, +radius.y, 0.0f);

    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);

    TextureQuadVertex *v = render_group->vertex_array + render_group->vertex_count;
    v[0] = {p0, uv0, c};
    v[1] = {p1, uv1, c};
    v[2] = {p2, uv2, c};
    v[3] = {p3, uv3, c};

    u16 *idx = render_group->index_array + render_group->index_count;
    u16 offset = render_group->vertex_count;
    idx[0] = offset + 0;
    idx[1] = offset + 1;
    idx[2] = offset + 2;
    idx[3] = offset + 2;
    idx[4] = offset + 3;
    idx[5] = offset + 0;

    render_group->vertex_count += 4;
    render_group->index_count += 6;
}
#endif