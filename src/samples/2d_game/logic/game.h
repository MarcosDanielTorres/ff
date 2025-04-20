#if 1
#pragma once

#include "game_types.h"
#include "math.h"

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
    f32 t;
    u32 next_node;
    b32 moving;
    b32 arrived;
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

struct SpawnPoint
{
    Vec2 p;
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

void add_vehicle(World *world, SpawnPoint sp, f32 w, f32 h)
{
    f32 x = sp.p.x;
    f32 y = sp.p.y;
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
    indexes are counterclockwise
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

global_variable u32 next_node = 10;
global_variable u32 start_node_idx = 0;
global_variable u32 arrived = false;
global_variable u32 vehicle_color = 0xFFFF00FF;
global_variable u32 node_color = 0xFF00FF0F;
global_variable u32 node_w = 20;
global_variable u32 h_node_w = node_w * 0.5;;

void move_vehicle(Entity *vehicle, Vec2 new_node_p, f32 dt)
{
    f32 dx = new_node_p.x - vehicle->p.x;
    f32 dy = new_node_p.y - vehicle->p.y;
    f32 dist = sqrtf(dx * dx + dy * dy);
    if(dist < 1.0f)
    {
        vehicle->moving = false;
        vehicle->p.x = new_node_p.x;
        vehicle->p.y = new_node_p.y;
    }
    else
    {
        dx /= dist;
        dy /= dist;
        // if i keep adding t im essentially accelerating the vehicle, which is not what i want yet
        //vehicle->t += dt; 
        vehicle->t = dt;
        vehicle->p.x = vehicle->p.x + vehicle->t * dx;
        vehicle->p.y = vehicle->p.y + vehicle->t * dy;
    }
}

void main_game_loop(OS_PixelBuffer *buffer, GameInput *input, GameMemory *memory)
{

    clear_buffer(buffer, 0xFF000000);
    GameState *game_state = (GameState*) memory->permanent_mem;
    if (!memory->init)
    {
        arena_init(&game_state->arena, memory->permanent_mem_size - sizeof(GameState), (u8*)(memory->permanent_mem_size + sizeof(GameState)));
        printf("init\n");

        // place a vehicle in one of the nodes
        u32 start_node_idx = 0;
        for(u32 i = 0; i < 1; i++)
        {
            SpawnPoint sp = {locations[start_node_idx]};
            add_vehicle(&game_state->world, sp, 20, 50);
        }

        add_player(&game_state->world, 500, 500, 10, 10, 0xFF00FFFF);

        memory->init = true;
    }

    World* world = &game_state->world;

    // update
    // move the vehicle to another different node
    u32 end_node_idx = 2;
    // perform DFS
    // should be split per various frames
    // - get next node
    // - move to that node periodically
    // - repeat until next_node == end_node!
    printf("Path: \n");
    /*
        So the tricky thing here is that while i can run this algho to get the final position
        I could care about the entire path including all nodes or i could just care for the next node in the path

        And i need to interpolate in various frames between the current node and the next to move them smoothly !
        Then when I arrive i need to start animating the next node until im at the end and i stop
    */
    Entity *vehicle = &world->entities[0];
    f32 dt = input->dt;
    while(next_node != end_node_idx && !vehicle->moving && !vehicle->arrived)
    {
        for(u32 neighbour_idx = 0; neighbour_idx < array_count(adj_list[0]); neighbour_idx++)
        {
            next_node = adj_list[start_node_idx][neighbour_idx];
            printf("From %d to %d\n", start_node_idx, next_node);

            vehicle->next_node = next_node;
            vehicle->moving = true;
            if (next_node == end_node_idx)
            {
                break;
            }
        }
        start_node_idx = next_node;
    }
    printf("---------- \n");

    f32 speed = 1.0f / 5.0f;
    speed = 100.0f;
    //if( vehicle->p.x == locations[vehicle->next_node].x &&
    //    vehicle->p.y == locations[vehicle->next_node].y )
    //{
    //    // I guess these dont toggle correctly as there are some precision subtleties that never allows these to be equal
    //    vehicle->moving = false;
    //}

    if( vehicle->p.x == locations[end_node_idx].x &&
        vehicle->p.y == locations[end_node_idx].y )
    {
        // I guess these dont toggle correctly as there are some precision subtleties that never allows these to be equal
        vehicle->arrived = true;
    }
    if(!vehicle->arrived)
    {
        move_vehicle(vehicle, locations[vehicle->next_node], dt * speed);
    }

    // draw
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
    }

    for(u32 ent_idx = 0; ent_idx < world->count; ent_idx++)
    {
        Entity *ent = &world->entities[ent_idx];
        switch(ent->type)
        {
            case EntityType_Vehicle:
            {
                Vec2 node_p = ent->p;
                draw_rect(buffer, node_p.x + 5, node_p.y + 5, 10, 10, vehicle_color);
                // draw_vehicle(buffer, ent, color?);
            } break;
        }
    }

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

#else

#pragma once

#include "game_types.h"
#include "math.h"

//---------------------------------------------
enum EntityType : u8
{
    EntityType_Vehicle,
    EntityType_Player,
};

union Vec2
{
    struct { f32 x, y; };
    f32 c[2];
};

struct Entity
{
    EntityType type;
    Vec2      p;
    u32       current_node;
    u32       next_node;
    bool      moving;
    bool      arrived;
    f32       speed;
    u32       color;
    u32       randIndex;
    u32       framec;
};

struct World
{
    u32    count;
    Entity entities[30];
};

struct SpawnPoint { Vec2 p; };

struct GameState
{
    Arena arena;
    World world;
};

// global PRNG index
global_variable u32 RandomNumberIndex;

static inline u32 random_u32_global(void)
{
    u32 r = RandomNumberTable[RandomNumberIndex % array_count(RandomNumberTable)];
    RandomNumberIndex = (RandomNumberIndex + 1) % array_count(RandomNumberTable);
    return r;
}

// random integer in [min, max]
static inline u32 random_range_global(u32 min, u32 max)
{
    u32 range = max - min + 1;
    return (random_u32_global() % range) + min;
}

internal Entity *add_entity(World *world, f32 x, f32 y, u32 start_node, u32 color = 0xFFFFFFFF)
{
    Entity *ent = &world->entities[world->count++];
    ent->p            = Vec2{x, y};
    ent->color        = color;
    ent->randIndex    = random_u32_global() % array_count(RandomNumberTable);
    ent->framec       = random_range_global(0, 500);
    ent->current_node = start_node;
    ent->next_node    = start_node;
    ent->moving       = false;
    ent->arrived      = false;
    ent->speed        = 200.0f;  // pixels per second
    return ent;
}

void add_player(World *world, f32 x, f32 y, u32 color)
{
    Entity *ent = add_entity(world, x, y, /*unused*/0, color);
    ent->type   = EntityType_Player;
}

void add_vehicle(World *world, SpawnPoint sp, u32 start_node)
{
    Entity *ent = add_entity(world, sp.p.x, sp.p.y, start_node, 0xFFFF00FF);
    ent->type   = EntityType_Vehicle;
}

// adjacency list for 4 nodes
// each row: neighbor indices
global_variable u32 adj_list[4][2] = {
    {1, 3},  // 0 -> 1,3
    {0, 2},  // 1 -> 0,2
    {1, 3},  // 2 -> 1,3
    {0, 2},  // 3 -> 0,2
};

// node positions
global_variable Vec2 locations[4] = {
    {350.0f, 100.0f},
    {350.0f, 500.0f},
    {950.0f, 500.0f},
    {950.0f, 100.0f},
};

void main_game_loop(OS_PixelBuffer *buffer, GameInput *input, GameMemory *memory)
{
    clear_buffer(buffer, 0xFF000000);
    GameState *gs = (GameState*)memory->permanent_mem;

    if (!memory->init)
    {
        arena_init(&gs->arena,
                   memory->permanent_mem_size - sizeof(GameState),
                   (u8*)(memory->permanent_mem_size + sizeof(GameState)));
        // spawn one vehicle at node 0
        SpawnPoint sp = { locations[0] };
        add_vehicle(&gs->world, sp, 0);
        add_player(&gs->world, 500, 500, 0xFF00FFFF);
        memory->init = true;
    }

    World *world = &gs->world;
    f32 dt = input->dt;
    u32 end_node = 2;

    // update each entity
    for(u32 i = 0; i < world->count; i++)
    {
        Entity &e = world->entities[i];
        if(e.type == EntityType_Vehicle)
        {
            // if not moving and not yet at final destination, pick next node
            if(!e.moving && !e.arrived)
            {
                if(e.current_node == end_node)
                {
                    e.arrived = true;
                }
                else
                {
                    // pick random neighbor
                    u32 *neis = adj_list[e.current_node];
                    u32 choice = random_range_global(0, 3);
                    e.next_node = neis[choice];
                    e.moving    = true;
                }
            }

            // move towards next_node if flagged
            if(e.moving && !e.arrived)
            {
                Vec2 target = locations[e.next_node];
                f32 dx = target.x - e.p.x;
                f32 dy = target.y - e.p.y;
                f32 dist = sqrtf(dx*dx + dy*dy);

                if(dist < 1.0f)
                {
                    // snap to node
                    e.p = target;
                    e.current_node = e.next_node;
                    e.moving = false;
                }
                else
                {
                    dx /= dist;
                    dy /= dist;
                    e.p.x += dx * e.speed * dt;
                    e.p.y += dy * e.speed * dt;
                }
            }

            // draw vehicle
            draw_rect(buffer, e.p.x-10, e.p.y-10, 20, 20, e.color);
        }
        else if(e.type == EntityType_Player)
        {
            draw_rect(buffer, e.p.x-5, e.p.y-5, 10, 10, e.color);
        }
    }

    // draw nodes and edges
    u32 node_w = 10;
    u32 half = node_w/2;
    for(u32 n = 0; n < 4; n++)
    {
        Vec2 np = locations[n];
        draw_rect(buffer, np.x-half, np.y-half, node_w, node_w, 0xFF00FF00);
        for(u32 j = 0; j < 2; j++)
        {
            Vec2 mp = locations[ adj_list[n][j] ];
            draw_line(buffer, np.x, np.y, mp.x, mp.y);
        }
    }
}

#endif