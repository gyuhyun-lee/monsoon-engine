#include "monsoon_world.h"
// TODO : Properly handle this value
#define WORLD_CHUNK_UNINITIALIZED_VALUE INT_MAX

internal void
InitializeWorld(world *World)
{
    for(u32 ChunkIndex = 0;
        ChunkIndex < ArrayCount(World->WorldChunks);
        ++ChunkIndex)
    {
        world_chunk *Chunk = World->WorldChunks + ChunkIndex;
        Chunk->ChunkX = WORLD_CHUNK_UNINITIALIZED_VALUE;
    }
}

internal world_chunk*
GetWorldChunk(world *World, u32 ChunkX, u32 ChunkY, u32 ChunkZ = 0)
{
    world_chunk *Result = 0;

    // NOTE : I am using internal hashing here

    // TODO : Better hash function lol
    u32 ChunkHashValue = 200*ChunkX + 322*ChunkY + 123*ChunkZ;
    // TODO : Better modding?
    u32 ChunkHashKey = ChunkHashValue%ArrayCount(World->WorldChunks);
    u32 OriginalChunkHashKey = ChunkHashKey;
    do
    {
        world_chunk *FirstInHash = World->WorldChunks + ChunkHashKey;
        if(FirstInHash->ChunkX == WORLD_CHUNK_UNINITIALIZED_VALUE)
        {
            Result = FirstInHash;
            Result->ChunkX = ChunkX;
            Result->ChunkY = ChunkY;
            Result->ChunkZ = ChunkZ;

            break;
        }
        else if(FirstInHash->ChunkX == ChunkX &&
            FirstInHash->ChunkY == ChunkY &&
            FirstInHash->ChunkZ == ChunkZ)
        {
            Result = FirstInHash;

            break;
        }

        ChunkHashKey = (ChunkHashKey + 1)%ArrayCount(World->WorldChunks);
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
    i32 ChunkOffsetX = RoundR32ToInt32(WorldPos->P.X/World->ChunkDim.X);
    i32 ChunkOffsetY = RoundR32ToInt32(WorldPos->P.Y/World->ChunkDim.Y);

    WorldPos->ChunkX += ChunkOffsetX;
    WorldPos->P.X -= ChunkOffsetX * World->ChunkDim.X;

    WorldPos->ChunkY += ChunkOffsetY;
    WorldPos->P.Y -= ChunkOffsetY*World->ChunkDim.Y;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos, v2 Delta)
{
    WorldPos->P += Delta;
    CanonicalizeWorldPos(World, WorldPos);
}
internal u32
ConvertMeterToTileCount(world *World, r32 ValueInMeter)
{
    u32 Result = 0;

    Result = (u32)(ValueInMeter/World->TileSideInMeters);

    return Result;
}

internal v2
WorldPositionDifferenceInMeter(world *World, world_position *B, world_position *A)
{
    v2 Diff = {};

    i32 ChunkDiffX = B->ChunkX - A->ChunkX;
    i32 ChunkDiffY = B->ChunkY - A->ChunkY;

    Diff.X = ChunkDiffX*World->ChunkDim.X + (B->P.X - A->P.X);
    Diff.Y = ChunkDiffY*World->ChunkDim.Y + (B->P.Y - A->P.Y);

    return Diff;
}

