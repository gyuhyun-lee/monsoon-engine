#include "monsoon_world.h"
// TODO : Properly handle this value
#define WORLD_CHUNK_UNINITIALIZED_VALUE INT_MAX

internal void
InitializeWorld(world *World)
{
    for(u32 ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->worldChunks);
        ++ChunkIndex)
    {
        world_chunk *Chunk = World->worldChunks + ChunkIndex;
        Chunk->chunkX = WORLD_CHUNK_UNINITIALIZED_VALUE;
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


internal world_chunk*
GetWorldChunk(world *World, u32 ChunkX, u32 ChunkY, u32 ChunkZ = 0)
{
    world_chunk *Result = 0;

    // NOTE : I am using internal hashing here

    // TODO : Better hash function lol
    u32 ChunkHashValue = 200*ChunkX + 322*ChunkY + 123*ChunkZ;
    // TODO : Better modding?
    u32 ChunkHashKey = ChunkHashValue%ArrayCount(World->worldChunks);
    u32 OriginalChunkHashKey = ChunkHashKey;
    do
    {
        world_chunk *FirstInHash = World->worldChunks + ChunkHashKey;
        if(FirstInHash->chunkX == WORLD_CHUNK_UNINITIALIZED_VALUE)
        {
            Result = FirstInHash;
            Result->chunkX = ChunkX;
            Result->chunkY = ChunkY;
            Result->chunkZ = ChunkZ;

            break;
        }
        else if(FirstInHash->chunkX == ChunkX &&
            FirstInHash->chunkY == ChunkY &&
            FirstInHash->chunkZ == ChunkZ)
        {
            Result = FirstInHash;

            break;
        }

        ChunkHashKey = (ChunkHashKey + 1)%ArrayCount(World->worldChunks);
    }
    while(ChunkHashKey != OriginalChunkHashKey);

    // TODO : What should we do if the world chunk is somehow empty??
    // Maybe if it is not urgent(i.e we don't need to simulate the entity inside this world chunk
    // for now), put onto the queue or something?
    Assert(Result);

    return Result;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos)
{
    i32 ChunkOffsetX = RoundR32ToInt32(WorldPos->p.x/World->chunkDim.x);
    i32 ChunkOffsetY = RoundR32ToInt32(WorldPos->p.y/World->chunkDim.y);

    WorldPos->chunkX += ChunkOffsetX;
    WorldPos->p.x -= ChunkOffsetX * World->chunkDim.x;

    WorldPos->chunkY += ChunkOffsetY;
    WorldPos->p.y -= ChunkOffsetY*World->chunkDim.y;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos, v2 Delta)
{
    WorldPos->p += Delta;
    CanonicalizeWorldPos(World, WorldPos);
}
internal u32
ConvertMeterToTileCount(world *World, r32 ValueInMeter)
{
    u32 Result = 0;

    Result = (u32)(ValueInMeter/World->tileSideInMeters);

    return Result;
}

internal v2
WorldPositionDifferenceInMeter(world *World, world_position *B, world_position *A)
{
    v2 Diff = {};

    i32 ChunkDiffX = B->chunkX - A->chunkX;
    i32 ChunkDiffY = B->chunkY - A->chunkY;

    Diff.x = ChunkDiffX*World->chunkDim.x + (B->p.x - A->p.x);
    Diff.y = ChunkDiffY*World->chunkDim.y + (B->p.y - A->p.y);

    return Diff;
}

