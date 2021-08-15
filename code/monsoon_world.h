#ifndef MONSOON_WORLD_H
#define MONSOON_WORLD_H

/*
 * NOTE : Here is how I will construct the world chunk.
 * 1. 
.XY *  Chunk is not variable sized(at least for now). If it is, there is too much to think
 *  how much the chunk size should vary, or what if the entity is not inside any of the chunk,
 *  should I make a new chunk(and also somehow know how big that chunk should be without interupting
 *  other chunks), or should I make any of the close chunk bigger...
 *  
 *  So the chunk is nice and evenly distributed with same sizes across the world. The game will just
 *  not simulate the chunks that are outside the boundary or if there is no entity that the game
 *  updates.
 *
 * 2. 
 *  To get the chunk X, Y, or Z from the Absolute Tile posiiton, because the chunks are evenly cut,
 *  we can just do AbsTile/ChunkDim;
 * 
 * 
 *
*/

// IMPORTANT : NOTE : Left Bottom corner of the world chunk with chunk pos (0, 0, 0) 
// is the center of the Cartesian coordinate system.
// left bottom corner of the tile with AbsTilePos (0, 0, 0) 
// is also the center of the Cartesian coordinate system.
// Also, tile (0, 0, 0)'s left bottom corner matches the left bottom corner of the chunk
struct world_position
{
    // NOTE : All chunks has same sizes and evenly distributed across the world.
    u32 chunkX;
    u32 chunkY;
    u32 chunkZ;

    // NOTE : This is relative to the center of the chunk
    v3 p;
};

enum entity_type
{
    EntityType_Null,
    EntityType_Player,
    EntityType_Wall,
};

struct low_entity
{
    entity_type type;
    world_position worldP;

    v3 dP;
    v3 dim;
};

struct low_entity_block
{
    // TODO : What might be the optimal value for entity block?
    // TODO : Change this to be double-linked list?
    low_entity *entities[16];
    u32 entityCount;

    low_entity_block *next;
};
struct world_chunk
{
    low_entity_block entityBlock;

    u32 chunkX;
    u32 chunkY;
    u32 chunkZ;
};

struct world
{
    // TODO : How many chunks are needed?
    // TODO : What should happen if we use all of the world chunks?
    world_chunk worldChunks[256];
    v3 chunkDim;

    r32 tileSideInMeters;
};


#endif
