#pragma once

#include "game_types.h"
#include "math.h"

#if 0 
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
    u32 start_node_idx;
    u32 end_node_idx;
    u32 parent;
    u32 visited[9];
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

void add_vehicle(World *world, SpawnPoint sp, f32 w, f32 h, u32 s, u32 e)
{
    f32 x = sp.p.x;
    f32 y = sp.p.y;
    Entity *ent = add_entity(world, x, y, w, h);
    ent->type = EntityType_Vehicle;
    ent->start_node_idx = s;
    ent->end_node_idx = e;
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
    0----3----8 
    |    |    | 
    1----2----7 
    |    |    | 
    4----5----6 
*/

global_variable i32 adj_list[9][4] =
{
    {1, 3, -1, -1},
    {0, 2,  4, -1},
    {1, 3,  5,  7},
    {0, 2,  8, -1},
    {1, 5, -1, -1},
    {2, 4,  6, -1},
    {5, 7, -1, -1},
    {2, 6,  8, -1},
    {3, 7, -1, -1},
};

global_variable Vec2 locations[9] =
{
    {150.0f, 50.0f},
    {150.0f, 350.0f},
    {600.0f, 350.0f},
    {600.0f, 50.0f},
    {150.0f, 600.0f},
    {600.0f, 600.0f},
    {1050.0f, 600.0f},
    {1050.0f, 350.0f},
    {1050.0f, 50.0f},
};

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
    clear_buffer(buffer, 0xFFffffff);
    GameState *game_state = (GameState*) memory->permanent_mem;
    if (!memory->init)
    {
        arena_init(&game_state->arena, memory->permanent_mem_size - sizeof(GameState), (u8*)(memory->permanent_mem_size + sizeof(GameState)));
        printf("init\n");

        // place a vehicle in one of the nodes
        for(u32 i = 0; i < 4; i++)
        {
            SpawnPoint sp = {locations[i]};
            add_vehicle(&game_state->world, sp, 20, 50, i, i + 2);
        }

        add_player(&game_state->world, 500, 500, 10, 10, 0xFF00FFFF);

        memory->init = true;
    }

    World* world = &game_state->world;

    // perform DFS
    // should be split per various frames
    // - get next node
    // - move to that node periodically
    // - repeat until next_node == end_node!
    /*
        So the tricky thing here is that while i can run this algho to get the final position
        I could care about the entire path including all nodes or i could just care for the next node in the path

        And i need to interpolate in various frames between the current node and the next to move them smoothly !
        Then when I arrive i need to start animating the next node until im at the end and i stop
    */
    f32 dt = input->dt;

    // in this algorithm im creating the path from node to node, meaning once i find one path i stop until the next calculation.
    // Another possible solution would be to create the entire path this frame given final_start and final_end
    // So i would end up with a list of node locations (path)
    // So this is a Frame-persistent DFS or stateful along frames DFS

    f32 speed = 200.0f;
    //if( vehicle->p.x == locations[vehicle->next_node].x &&
    //    vehicle->p.y == locations[vehicle->next_node].y )
    //{
    //    // I guess these dont toggle correctly as there are some precision subtleties that never allows these to be equal
    //    vehicle->moving = false;
    //}


    for(u32 node_idx = 0; node_idx < array_count(adj_list); node_idx++)
    {
        Vec2 node_p = locations[node_idx];
        draw_rect(buffer, node_p.x, node_p.y, node_w, node_w, node_color);
        for(u32 neighbour = 0; neighbour < array_count(adj_list[0]); neighbour++)
        {
            u32 neighbour_idx = adj_list[node_idx][neighbour];
            if(neighbour_idx == -1)
            {
                break;
            }
            Vec2 neighbour_p = locations[neighbour_idx];
            draw_line(buffer, node_p.x + h_node_w, node_p.y + h_node_w, neighbour_p.x + h_node_w, neighbour_p.y + h_node_w, 0);
        }
    }

    for(u32 ent_idx = 0; ent_idx < world->count; ent_idx++)
    {
        Entity *ent = &world->entities[ent_idx];
        switch(ent->type)
        {
            case EntityType_Vehicle:
            {
                while(!ent->moving && !ent->arrived)
                {
                    b32 no_more_neighs = 0;
                    for(u32 neighbour_idx = 0; neighbour_idx < array_count(adj_list[0]); neighbour_idx++)
                    {
                        if(adj_list[ent->start_node_idx][neighbour_idx] == -1)
                        {
                            ent->start_node_idx = ent->parent;
                            no_more_neighs = 1;
                            break;
                        }
                        u32 next_node = adj_list[ent->start_node_idx][neighbour_idx];
                        printf("From %d to %d\n", ent->start_node_idx, next_node);

                        if (ent->visited[next_node]) continue;
                        ent->next_node = next_node;
                        ent->visited[next_node] = 1;
                        ent->moving = true;
                        break;
                    }
                    if(!no_more_neighs)
                    {
                        ent->parent = ent->start_node_idx;
                        ent->start_node_idx = ent->next_node;
                    }
                }

                if(!ent->arrived)
                {
                    move_vehicle(ent, locations[ent->next_node], dt * speed);
                }

                if( ent->p.x == locations[ent->end_node_idx].x &&
                    ent->p.y == locations[ent->end_node_idx].y )
                {
                    ent->arrived = true;
                    memset(ent->visited, 0, sizeof(ent->visited));
                }

            } break;
        }

        switch(ent->type)
        {
            case EntityType_Vehicle:
            {
                Vec2 node_p = ent->p;
                draw_rect(buffer, node_p.x + 5, node_p.y + 5, 10, 10, vehicle_color);
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


#include <stdlib.h>
#include <math.h>
#include <float.h>

#define NODES 9
#define MAX_DEGREE 4

// your adjacency list:
int adj[NODES][MAX_DEGREE] = {
  {1,3,-1,-1}, {0,2,4,-1}, {1,3,5,7},
  {0,2,8,-1}, {1,5,-1,-1}, {2,4,6,-1},
  {5,7,-1,-1}, {2,6,8,-1}, {3,7,-1,-1}
};

// 2D positions for heuristic:
float px[NODES] = {150,150,600,600,150,600,1050,1050,1050};
float py[NODES] = { 50,350,350, 50,600,600, 600, 350,  50};

// ——— simple dynamic array for ints ———
typedef struct {
    int *data;
    int  count, cap;
} IntArray;

void initArr(IntArray *a) {
    a->count = 0;
    a->cap   = 4;
    a->data  = (int*)malloc(a->cap * sizeof *a->data);
}

void pushArr(IntArray *a, int v) {
    if (a->count == a->cap) {
        a->cap *= 2;
        a->data = (int*)realloc(a->data, a->cap * sizeof *a->data);
    }
    a->data[a->count++] = v;
}

void freeArr(IntArray *a) {
    free(a->data);
}

// ——— A* on 3×3 ———
int heuristic(int a, int b) {
    // Euclidean:
    float dx = px[a] - px[b];
    float dy = py[a] - py[b];
    return (int)(sqrtf(dx*dx + dy*dy));
}

int a_star(int start, int goal, IntArray *out_path) {
    // scores
    int g[NODES];           // cost so far
    int f[NODES];           // g+h
    int parent[NODES];      // back‐pointers
    int inOpen[NODES]  = {0};
    int inClosed[NODES]= {0};

    // init
    for (int i = 0; i < NODES; i++) {
        g[i]      = INT_MAX;
        f[i]      = INT_MAX;
        parent[i] = -1;
    }

    g[start] = 0;
    f[start] = heuristic(start, goal);

    // open set as a flat list (scan for min f):
    IntArray open; initArr(&open);
    pushArr(&open, start);
    inOpen[start] = 1;

    while (open.count) {
        // find index of lowest‑f node
        int best_i = 0;
        for (int i = 1; i < open.count; i++) {
            if (f[open.data[i]] < f[open.data[best_i]])
                best_i = i;
        }
        int current = open.data[best_i];

        // remove it from open[] by swapping with last
        open.data[best_i] = open.data[--open.count];
        inOpen[current] = 0;

        if (current == goal) {
            // reconstruct path
            IntArray rev; initArr(&rev);
            for (int at = goal; at != -1; at = parent[at])
                pushArr(&rev, at);
            // reverse into out_path
            for (int i = rev.count - 1; i >= 0; i--)
                pushArr(out_path, rev.data[i]);
            freeArr(&rev);
            freeArr(&open);
            return 1;
        }

        inClosed[current] = 1;

        // examine neighbors
        for (int ni = 0; ni < MAX_DEGREE; ni++) {
            int nbr = adj[current][ni];
            if (nbr < 0) break;
            if (inClosed[nbr]) continue;

            int tentative_g = g[current] + 1;  // every edge costs 1
            if (!inOpen[nbr]) {
                // discover a new node
                inOpen[nbr] = 1;
                pushArr(&open, nbr);
            } else if (tentative_g >= g[nbr]) {
                continue;
            }

            // best path so far
            parent[nbr] = current;
            g[nbr]      = tentative_g;
            f[nbr]      = tentative_g + heuristic(nbr, goal);
        }
    }

    freeArr(&open);
    return 0;  // no path
}

struct GameState
{
    Arena arena;
    Arena transient_arena;
    FontInfo font_info;
};

static Str8 node_names[NODES];

void noop(Arena *arena, Str8 string)
{
    TempArena temp = temp_begin(arena);
    Str8 noop = str8_fmt(temp.arena, "noop\n");
    temp_end(temp);
}

void starting(Arena *arena)
{
    TempArena temp = temp_begin(arena);
    Str8 string = str8_fmt(temp.arena, "1\n");
    // esto esta bien pero si quisiera usar un supuesta valor retornado por noop
    //  
    noop(temp.arena, string);
    temp_end(temp);
}

void main_game_loop(OS_PixelBuffer *buffer, GameInput *input, GameMemory *memory)
{
    clear_buffer(buffer, 0xFF000000);
    GameState* game_state = (GameState*) memory->permanent_mem;
    if (!memory->init)
    {
        arena_init(&game_state->arena, memory->permanent_mem_size - sizeof(GameState), (u8*)(memory->permanent_mem + sizeof(GameState)));
        arena_init(&game_state->transient_arena, memory->transient_mem_size, (u8*)(memory->transient_mem));

        font_init();
        game_state->font_info = font_load(&game_state->arena);

        for(u32 i = 0; i < NODES; i++)
        {
            // TODO
            //node_names[i] = str8_fmt();
        }
        starting(&game_state->transient_arena);
        memory->init = true;
    }
    TempArena temp_arena = temp_begin(&game_state->transient_arena);

    IntArray path; initArr(&path);
    u32 start = 0;
    u32 end = 4;
    a_star(start, end, &path);

    float px[NODES] = {150,150,600,600,150,600,1050,1050,1050};
    float py[NODES] = { 50,350,350, 50,600,600, 600, 350,  50};

    for(u32 i = 0; i < NODES; i++)
    {
        for(u32 n = 0; n < MAX_DEGREE; n++)
        {
            u32 neigh = adj[i][n];
            if(neigh == -1) continue;
            draw_line(buffer, px[i] + 5, py[i] + 5, px[neigh] + 5, py[neigh] + 5, 0xFFFFFFFF);
        }
        draw_rect(buffer, px[i], py[i], 10, 10, 0xFF00F0F);

        // Ver esto porque una cosa es llamar un temp_begin con unas arenas en el transient
        // y otra es directamente pasar el transient y sacar un temp begin de adentro (que esto es lo que hace raddebug)
        // Ellos agarran una transient (que no era un temp begin) y hacen un temp begin con esta trasient. YO lo que hago
        // es que agarro la transient, le creo un temp begin y despues voy y creo otro mas adentro. Encadeno los temp begins
        // Esto raddbg seguramente lo hace tambien, tendria que ver exactamente la distincion!!!
        // Lo que espero es que cuando yo salgo de str8_fmt este copiado en el temp begin de arriba la len y todo, que no pasa porque son la misma mierda
        // las dos

        // es importante que dentro de un begin/end temp no se meta otro bloque begin/end temp con la misma arena. tiene que ser otra!
        // buscar eejmplos de esto en la codebase

        /*
            TODO: Cuando dije esto mas arriba: "es importante que dentro de un begin/end temp no se meta otro bloque begin/end temp con la misma arena. tiene que ser otra!" 
            Despues revisando el codigo de rad me di cuenta de que esto sucede ahi. Lo que tengo que hacer es ver que problema tenia, y como es diferente
            a lo que hacen en rad al menos en `set_thread_namef`.

            Mi problema era que: Yo queria dejar esto persistente en la arena permanente, en la funcion `set_thread_namef` la memoria
            se usa dentro del stack y despues desaparece. Tendria que hacer un ejemplo. Literalmente puedo implementar lo mismo que hicieron


        
            void noop(Arena *arena, Str8 string)
            {
                TempArena temp = temp_begin(arena);
                Str8 noop = str8_fmt(temp.arena);
                temp_end(temp);
            }

            void starting(Arena *arena, Str8 string)
            {
                TempArena temp = temp_begin(arena);
                Str8 string = str8_fmt(temp.arena);
                noop(string);
                temp_end(temp);
            }
        */

        draw_text(buffer, px[i] + 5, py[i] - 10, str8_fmt(temp_arena.arena, "%d", i), game_state->font_info.font_table);
        //draw_text(buffer, px[i] + 5, py[i] - 10, str8_fmt(temp_arena.arena, "%d", i), game_state->font_info.font_table);
        //draw_text(buffer, px[i] + 5, py[i] - 10, node_name[i], i), game_state->font_info.font_table);
    }

    u32 yellow = 0xFFFFFF00;

    // draw path
    u32 path_sum = 0;
    for(u32 i = 0; i < path.count - 1; i++)
    {
        u32 c_node_idx = path.data[i];
        u32 n_node_idx = path.data[i + 1];
        //path_sum += dist between nodes??
        draw_line(buffer, px[c_node_idx] + 5, py[c_node_idx] + 5, px[n_node_idx] + 5, py[n_node_idx] + 5, yellow);
    }
    temp_end(temp_arena);
}

#endif