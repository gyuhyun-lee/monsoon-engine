#ifndef MONSOON_WORLD_H
#define MONSOON_WORLD_H

/*
 * NOTE : Here is how I will construct the world chunk.
 * 1. 
 *  Chunk is not variable sized(at least for now). If it is, there is too much to think
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
struct world_position
{
    // NOTE : All chunks has same sizes and evenly distributed across the world.
    u32 ChunkX;
    u32 ChunkY;
    u32 ChunkZ;

    // NOTE : This is relative to the center of the chunk
    v2 P;
};


enum entity_type
{
    EntityType_Null,
    EntityType_Player,
    EntityType_Wall,
};

struct low_entity
{
    entity_type Type;
    world_position WorldP;

    v2 dP;
    v2 Dim;
};

struct low_entity_block
{
    // TODO : What might be the optimal value for entity block?
    // TODO : Change this to be double-linked list?
    low_entity *Entities[16];
    u32 EntityCount;

    low_entity_block *Next;
};
struct world_chunk
{
    low_entity_block EntityBlock;

    u32 ChunkX;
    u32 ChunkY;
    u32 ChunkZ;
};

struct world
{
    // TODO : How many chunks are needed?
    world_chunk WorldChunks[256];
    v2 ChunkDim;

    r32 TileSideInMeters;
};


#endif
