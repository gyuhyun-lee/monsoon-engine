#ifndef FOX_WORLD_H
#define FOX_WORLD_H

// NOTE : Might be able to _borrow_ entity slot from adjacent world chunk!
struct world_p
{
    i32 chunkX;
    i32 chunkY;
    i32 chunkZ;
    
    // IMPORTANT : 0, 0, 0 -> chunkDim based!!!
    v3 offset;
};

struct sleeping_entity;
struct world_chunk
{
    /*Identifiers*/
    i32 chunkX;
    i32 chunkY;
    i32 chunkZ;

    sleeping_entity *ses[16];
    u32 seCount;

    // NOTE : This is external hash, because next hashes are chained externally
    // using the good old pointer
    world_chunk *nextInHash;
};

struct game_world
{
    // TODO : This must be power of two.
    /*
        Thanks to this being power of two, 
        we can do this
        hash value & (ArrayCount(worldChunkHash) - 1);

        because (power of two-1) is 111....111 -> which we can mask 
        our hash Value to get a hash key that is within the array size.
    */
    world_chunk chunkHashes[4096];

    r32 tileSizeInMeter;

    // u32 worldChunkSizeX;
    // u32 worldChunkSizeY;
    // u32 worldChunkSizeZ;

    v3 chunkDim;
};

#endif