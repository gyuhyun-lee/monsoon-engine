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
    u32 ChunkHashKey = ChunkHashValue%ArrayCount(World->WorldChunks);

    world_chunk *FirstInHash = World->WorldChunks + ChunkHashKey;
    world_chunk *NextInHash = 0;

    // TODO : Maybe there is a cleaner way to handle this? i.e do-while loop
    if(FirstInHash->ChunkX == WORLD_CHUNK_UNINITIALIZED_VALUE)
    {
        Result = FirstInHash;
        Result->ChunkX = ChunkX;
        Result->ChunkY = ChunkY;
        Result->ChunkZ = ChunkZ;
    }
    else if(FirstInHash->ChunkX == ChunkX &&
        FirstInHash->ChunkY == ChunkY &&
        FirstInHash->ChunkZ == ChunkZ)
    {
        Result = FirstInHash;
    }
    else
    {
        NextInHash = FirstInHash + 1;
        while(NextInHash->ChunkX != WORLD_CHUNK_UNINITIALIZED_VALUE)
        {
            if(NextInHash->ChunkX == ChunkX &&
                NextInHash->ChunkY == ChunkY &&
                NextInHash->ChunkZ == ChunkZ)
            {
                Result = NextInHash;
                break;
            }       
            else
            {
                // NOTE : Internal hash chain
                NextInHash++; 
            }
        }

        if(!Result)
        {
            Result = NextInHash;
            Result->ChunkX = ChunkX;
            Result->ChunkY = ChunkY;
            Result->ChunkZ = ChunkZ;
        }
    }

    // TODO : What should we do if the world chunk is somehow empty??
    // Maybe if it is not urgent(i.e we don't need to simulate the entity inside this world chunk
    // for now), put onto the queue or something?
    Assert(Result);

    return Result;
}

#if 0
// NOTE : TileX and TileY should be canonicalized
internal u32
GetTileValueFromWorldChunkUnchecked(world_chunk *WorldChunk, u32 TileX, u32 TileY)
{
    Assert(TileX >= WorldChunk->MinAbsTileX && 
            TileY >= WorldChunk->MinAbsTileY && 
            TileX < WorldChunk->MaxAbsTileX && 
            TileY < WorldChunk->MaxAbsTileY);

    u32 TileCountX = WorldChunk->MaxAbsTileX - WorldChunk->MinAbsTileX;
    u32 TileCountY = WorldChunk->MaxAbsTileY - WorldChunk->MinAbsTileY;

    u32 WorldChunkRelTileX = TileX - WorldChunk->MinAbsTileX;
    u32 WorldChunkRelTileY = TileY - WorldChunk->MinAbsTileY;

    u32 Result = WorldChunk->Tiles[TileCountX * (TileCountY - WorldChunkRelTileY - 1) + WorldChunkRelTileX];
    return Result;
}

internal b32 
IsTileEmptyUnchecked(world_chunk *WorldChunk, u32 AbsTileX, u32 AbsTileY)
{
    b32 Result = false;

    if(!GetTileValueFromWorldChunkUnchecked(WorldChunk, AbsTileX, AbsTileY))
    {
        Result = true;
    }

    return Result;
}

// NOTE : This function always needs canonicalized position
internal b32
IsWorldPointEmptyUnchecked(world *World, world_position CanPos)
{
    b32 Result = false;

    world_chunk *WorldChunk = GetWorldChunk(World, CanPos.AbsTileX, CanPos.AbsTileY);
    if(WorldChunk)
    {
        Result = IsTileEmptyUnchecked(WorldChunk, CanPos.AbsTileX, CanPos.AbsTileY);
    }

    return Result;
}
#endif


// TODO : This should work in chunk world, not inside tile
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

