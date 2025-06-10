struct TextureQuadVertex
{
    glm::vec3 p;
    glm::vec2 uv;
    glm::vec4 c;
    f32 omit_texture;
    f32 corner_radius;
    f32 border_thickness;
};

struct GlyphVertex
{
    glm::vec3 p;
    glm::vec2 uv;
};

struct RenderGroupNode
{
    TextureQuadVertex *vertex_array;
};

struct RenderGroupChunkNode
{
    RenderGroupNode *first;
    RenderGroupNode *last;
    u32 count;
};

struct RenderGroupList
{
    RenderGroupChunkNode *next;

    u32 node_count;
    u32 total_vertex_count;
};

struct RenderGroup
{
    TextureQuadVertex *vertex_array;
    u32 vertex_count;
    u16 *index_array;
    u32 index_count;
    u32 max_vertex_count;
    u32 max_index_count;
};