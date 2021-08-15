#include "monsoon_world.h"
// TODO : Properly handle this value
#define WORLD_CHUNK_UNINITIALIZED_VALUE INT_MAX

internal void
InitializeWorld(world *World)
{
    u32 worldChunkCount = ArrayCount(World->worldChunks);
    for(u32 ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->worldChunks);
        ++ChunkIndex)
    {
        world_chunk *Chunk = World->worldChunks + ChunkIndex;
        Chunk->chunkX = WORLD_CHUNK_UNINITIALIZED_VALUE;
        Chunk->chunkY = WORLD_CHUNK_UNINITIALIZED_VALUE;
        Chunk->chunkZ = WORLD_CHUNK_UNINITIALIZED_VALUE;
    }
}

internal void
PutEntityInsideWorldChunk(world_chunk *worldChunk, memory_arena *arena, low_entity *Entity)
{
    low_entity_block *entityBlock = 0;
    low_entity_block *Search = &worldChunk->entityBlock;
    while(!entityBlock)
    {
        if(Search->entityCount < ArrayCount(worldChunk->entityBlock.entities))
        {
            entityBlock = Search;
        }
        else
        {
            if(Search->next)
            {
                Search = Search->next;
            }
            else
            {
                entityBlock = PushStruct(arena, low_entity_block);
                Search->next = entityBlock;
            }
        }
    }

    entityBlock->entities[entityBlock->entityCount++] = Entity; 
}


// NOTE : Using internal hashing here
internal world_chunk*
GetWorldChunk(world *World, u32 chunkX, u32 chunkY, u32 chunkZ, b32 shouldCreate = 0)
{
    world_chunk *result = 0;

    // TODO : Better hash function lol
    u32 ChunkHashValue = 200*chunkX + 322*chunkY + 123*chunkZ;
    // TODO : Better modding?
    u32 ChunkHashKey = ChunkHashValue%ArrayCount(World->worldChunks);
    u32 OriginalChunkHashKey = ChunkHashKey;

    world_chunk *emptyWorldChunk = 0;
    do
    {
        world_chunk *FirstInHash = World->worldChunks + ChunkHashKey;
        if(FirstInHash->chunkX == chunkX &&
            FirstInHash->chunkY == chunkY &&
            FirstInHash->chunkZ == chunkZ)
        {
            result = FirstInHash;

            break;
        }

        if(shouldCreate && !emptyWorldChunk)
        {
            if(FirstInHash->chunkX == WORLD_CHUNK_UNINITIALIZED_VALUE)
            {
                emptyWorldChunk = FirstInHash;
            }
        }

        ChunkHashKey = (ChunkHashKey + 1)%ArrayCount(World->worldChunks);
    }
    while(ChunkHashKey != OriginalChunkHashKey);

    if(shouldCreate && !result)
    {
        result = emptyWorldChunk;
        result->chunkX = chunkX;
        result->chunkY = chunkY;
        result->chunkZ = chunkZ;
    }

    // TODO : What should we do if the world chunk is somehow empty??
    // Maybe if it is not urgent(i.e we don't need to simulate the entity inside this world chunk
    // for now), put onto the queue or something?

    return result;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos)
{
    i32 ChunkOffsetX = RoundR32ToInt32(WorldPos->p.x/World->chunkDim.x);
    i32 ChunkOffsetY = RoundR32ToInt32(WorldPos->p.y/World->chunkDim.y);
    i32 ChunkOffsetZ = RoundR32ToInt32(WorldPos->p.z/World->chunkDim.z);

    WorldPos->chunkX += ChunkOffsetX;
    WorldPos->p.x -= ChunkOffsetX * World->chunkDim.x;

    WorldPos->chunkY += ChunkOffsetY;
    WorldPos->p.y -= ChunkOffsetY*World->chunkDim.y;

    WorldPos->chunkZ += ChunkOffsetZ;
    WorldPos->p.z -= ChunkOffsetZ*World->chunkDim.z;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos, v3 Delta)
{
    WorldPos->p += Delta;
    CanonicalizeWorldPos(World, WorldPos);
}
internal u32
ConvertMeterToTileCount(world *World, r32 ValueInMeter)
{
    u32 result = 0;

    result = (u32)(ValueInMeter/World->tileSideInMeters);

    return result;
}

internal v3
WorldPositionDifferenceInMeter(world *World, world_position *B, world_position *A)
{
    v3 Diff = {};

    i32 ChunkDiffX = B->chunkX - A->chunkX;
    i32 ChunkDiffY = B->chunkY - A->chunkY;
    i32 ChunkDiffZ = B->chunkZ - A->chunkZ;

    Diff.x = ChunkDiffX*World->chunkDim.x + (B->p.x - A->p.x);
    Diff.y = ChunkDiffY*World->chunkDim.y + (B->p.y - A->p.y);
    Diff.z = ChunkDiffZ*World->chunkDim.z + (B->p.z - A->p.z);

    return Diff;
}

