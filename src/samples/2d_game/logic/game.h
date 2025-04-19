#pragma once

#include "game_types.h"


enum EntityType : u8
{
    EntityType_Vehicle,
    EntityType_Player,
};

union Vec2
{
    struct
    {
        f32 x, y;
    };
    f32 c[2];
};

struct Entity
{
    EntityType type;
    Vec2 p;
    f32 dx;
    f32 dy;
    u32 color;
    u32 randIndex;
    u32 framec;
};

struct World
{
    u32 count;
    Entity entities[30];
};

struct GameState
{
    Arena arena;
    World world;
};

static inline u32 random_u32_global(void);

u32 random_range_global(u32 min, u32 max)
{
    u32 range = max - min + 1;
    return (random_u32_global() % range) + min;
}
internal Entity *
add_entity(World *world, f32 x, f32 y, f32 w, f32 h, u32 color = 0xFFFFFFFF)
{
    Entity *ent = &world->entities[world->count++];
    ent->p = Vec2{x, y};
    ent->color = color;
    ent->randIndex = random_u32_global() % array_count(RandomNumberTable);
    ent->framec = random_range_global(0, 500);
    return ent;
}

void add_player(World *world, f32 x, f32 y, f32 w, f32 h, u32 color)
{
    Entity *ent = add_entity(world, x, y, w, h, color);
    ent->type = EntityType_Player;
}

void add_vehicle(World *world, f32 x, f32 y, f32 w, f32 h)
{
    Entity *ent = add_entity(world, x, y, w, h);
    ent->type = EntityType_Vehicle;
}

global_variable u32 RandomNumberIndex;


// grab one U32 and advance the global index
static inline u32
random_u32_global(void)
{
    u32 r = RandomNumberTable[RandomNumberIndex % array_count(RandomNumberTable)];
    RandomNumberIndex = (RandomNumberIndex + 1) % array_count(RandomNumberTable);
    return r;
}

// grab one U32 and advance the entity's own index
static inline u32
random_u32_for(Entity *ent)
{
    u32 r = RandomNumberTable[ent->randIndex % array_count(RandomNumberTable)];
    ent->randIndex = (ent->randIndex + 1) % array_count(RandomNumberTable);
    return r;
}

// returns –1, 0 or +1 using global PRNG (not used for vehicles now)
static inline f32
random_step3_global(void)
{
    return (f32)(random_u32_global() % 3) - 1.0f;
}

// returns –1, 0 or +1 using an entity's own PRNG
static inline f32
random_step3_for(Entity *ent)
{
    return (f32)(random_u32_for(ent) % 3) - 1.0f;
}

/*


    0---------3
    |         |
    |         |
    |         |
    1---------2
*/

global_variable u32 adj_list[4][2] =
{
    {1, 3},
    {0, 2},
    {1, 3},
    {2, 0}
};

global_variable Vec2 locations[4] =
{
    {350.0f, 100.0f},
    {350.0f, 500.0f},
    {950.0f, 500.0f},
    {950.0f, 100.0f}
};

global_variable u32 vehicle_color = 0xFFFF00FF;
global_variable u32 node_color = 0xFF00FF0F;
global_variable u32 node_w = 20;
global_variable u32 h_node_w = node_w * 0.5;;
void main_game_loop(OS_PixelBuffer *buffer, GameInput *input, GameMemory *memory)
{

    clear_buffer(buffer, 0xFF000000);
    GameState *game_state = (GameState*) memory->permanent_mem;
    if (!memory->init)
    {
        arena_init(&game_state->arena, memory->permanent_mem_size - sizeof(GameState), (u8*)(memory->permanent_mem_size + sizeof(GameState)));
        printf("init\n");
        for(u32 i = 0; i < 2; i++)
        {
            add_vehicle(&game_state->world, 100 + i * 10, 100 - i * 4, 20, 50);
        }

        add_player(&game_state->world, 500, 500, 10, 10, 0xFF00FFFF);

        memory->init = true;
    }

    for(u32 node_idx = 0; node_idx < array_count(adj_list); node_idx++)
    {
        Vec2 node_p = locations[node_idx];
        draw_rect(buffer, node_p.x, node_p.y, node_w, node_w, node_color);
        for(u32 neighbour = 0; neighbour < array_count(adj_list[0]); neighbour++)
        {
            u32 neighbour_idx = adj_list[node_idx][neighbour];
            Vec2 neighbour_p = locations[neighbour_idx];
            printf("Node %d, neighbour: %d\n", node_idx, neighbour_idx);
            draw_line(buffer, node_p.x + h_node_w, node_p.y + h_node_w, neighbour_p.x + h_node_w, neighbour_p.y + h_node_w);
        }
        //draw_rect(buffer, node_p.x + 5, node_p.y + 5, 10, 10, vehicle_color);
    }

    /*  TODOs
        place a vehicle in one of the nodes
        move it to another different node
    */

    /*
    for(u32 i = 0; i < game_state->world.count; i++)
    {
        Entity *ent = game_state->world.entities + i;
        if (ent->type == EntityType_Player)
        {
            draw_rect(buffer, ent->p.x, ent->p.y, 10, 10, ent->color);
        }
        else
        {
            if (ent->framec < 500)
            {
                ent->framec++;
            }
            else
            {
                f32 dx = random_step3_for(ent);
                f32 dy = random_step3_for(ent); 
                ent->dx = dx;
                ent->dy = dy;
                ent->framec = 0;
            }

            printf("dx = %f\n", ent->dx);

            ent->p.x += ent->dx * input->dt * 150;
            ent->p.y += ent->dy * input->dt * 105;
            if (ent->p.x < 0)                ent->p.x = 0;
            if (ent->p.x > buffer->width-10) ent->p.x = buffer->width - 10;
            if (ent->p.y < 0)                ent->p.y = 0;
            if (ent->p.y > buffer->height-10)ent->p.y = buffer->height - 10;
            draw_rect(buffer, ent->p.x, ent->p.y, 10, 10);
        }
    }
    */
}