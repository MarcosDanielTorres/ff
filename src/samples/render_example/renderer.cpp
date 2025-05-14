
struct UIVertex
{
    glm::vec3 p;
    glm::vec2 uv;
    glm::vec4 c;
};

struct UIRenderGroup
{
    GLint ortho_proj;
    GLint texture_sampler;
    u32 program_id; 

    u32 vbo, vao, ebo;
    GLuint tex;
    UIVertex *vertex_array;
    u32 vertex_count;
    u16 *index_array;
    u32 index_count;
};

static u32 max_index_per_batch = 600;
static u32 max_vertex_per_batch = 600;

// Vertex ordering
// x+ right, y+ up, z- into the screen

void check_bounds(u32 vertex_count, u32 index_count)
{
    AssertGui(vertex_count < max_vertex_per_batch, "max vertex count per batch exceeded! %d\n", vertex_count);
    AssertGui(index_count < max_index_per_batch, "max index count per batch exceeded! %d\n", index_count);
}

void push_triangle(UIRenderGroup *render_group, glm::vec3 tri_points[3], u16 tri_indices[3])
{
    check_bounds(3 + render_group->vertex_count, 3 + render_group->index_count);

    UIVertex *v = render_group->vertex_array + render_group->vertex_count;
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

void push_rect(UIRenderGroup *render_group, const glm::vec3 quad_points[4])
{
    check_bounds(4 + render_group->vertex_count, 6 + render_group->index_count);

    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);

    glm::vec4 c = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    UIVertex *v = render_group->vertex_array + render_group->vertex_count;
    v[0] = {quad_points[0], uv0, c};
    v[1] = {quad_points[1], uv1, c};
    v[2] = {quad_points[2], uv2, c};
    v[3] = {quad_points[3], uv3, c};

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

// NOTE Shouldnt even use this one its calculating the position inside of the function. which is not right
void push_quad(UIRenderGroup *render_group, glm::vec3 center, glm::vec2 radius, glm::vec4 c)
{
    check_bounds(4 + render_group->vertex_count, 6 + render_group->index_count);

    glm::vec3 p0 = center + glm::vec3(-radius.x, -radius.y, 0.0f);
    glm::vec3 p1 = center + glm::vec3(+radius.x, -radius.y, 0.0f);
    glm::vec3 p2 = center + glm::vec3(+radius.x, +radius.y, 0.0f);
    glm::vec3 p3 = center + glm::vec3(-radius.x, +radius.y, 0.0f);

    glm::vec2 uv0 = glm::vec2(0, 0);
    glm::vec2 uv1 = glm::vec2(1, 0);
    glm::vec2 uv2 = glm::vec2(1, 1);
    glm::vec2 uv3 = glm::vec2(0, 1);

    UIVertex *v = render_group->vertex_array + render_group->vertex_count;
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