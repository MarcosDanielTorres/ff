This should be with ui notes (wherever that may be!)



Entities:

55: 
    f
56: 
57: 



struct entity_block
{
    u32 count;
    u32 low_entity_index[16];
    entity_block *next;
};

struct chunk
{
    entity_block first_block;
    chunk *NextInHash
};

struct tilemap
{
    chunk chunkhash[4096];
};