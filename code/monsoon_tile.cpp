internal tile_chunk*
GetTileChunk(world *World, u32 TileX, u32 TileY)
{
    tile_chunk *Result = 0;

    // TODO : Hashing instead of looping all the tile chunks
    for(u32 TileChunkIndex = 0;
        TileChunkIndex < ArrayCount(World->TileChunks);
        ++TileChunkIndex)
    {
        tile_chunk *TileChunk = World->TileChunks + TileChunkIndex;
        
        if(TileX >= TileChunk->MinAbsTileX && TileX < TileChunk->MaxAbsTileX &&
            TileY >= TileChunk->MinAbsTileY && TileY < TileChunk->MaxAbsTileY)
        {
            Result = TileChunk;
            break;
        }
    }

    return Result;
}

// NOTE : TileX and TileY should be canonicalized
internal u32
GetTileValueFromTileChunkUnchecked(tile_chunk *TileChunk, u32 TileX, u32 TileY)
{
    Assert(TileX >= TileChunk->MinAbsTileX && 
            TileY >= TileChunk->MinAbsTileY && 
            TileX < TileChunk->MaxAbsTileX && 
            TileY < TileChunk->MaxAbsTileY);

    u32 TileCountX = TileChunk->MaxAbsTileX - TileChunk->MinAbsTileX;
    u32 TileCountY = TileChunk->MaxAbsTileY - TileChunk->MinAbsTileY;

    u32 TileChunkRelTileX = TileX - TileChunk->MinAbsTileX;
    u32 TileChunkRelTileY = TileY - TileChunk->MinAbsTileY;

    u32 Result = TileChunk->Tiles[TileCountX * (TileCountY - TileChunkRelTileY - 1) + TileChunkRelTileX];
    return Result;
}

internal b32 
IsTileEmptyUnchecked(tile_chunk *TileChunk, u32 AbsTileX, u32 AbsTileY)
{
    b32 Result = false;

    if(!GetTileValueFromTileChunkUnchecked(TileChunk, AbsTileX, AbsTileY))
    {
        Result = true;
    }

    return Result;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos)
{
    i32 TileOffsetX = RoundR32ToInt32(WorldPos->P.X/World->TileSideInMeters);
    i32 TileOffsetY = RoundR32ToInt32(WorldPos->P.Y/World->TileSideInMeters);

    WorldPos->AbsTileX += TileOffsetX;
    WorldPos->P.X -= TileOffsetX * World->TileSideInMeters;

    WorldPos->AbsTileY += TileOffsetY;
    WorldPos->P.Y -= TileOffsetY*World->TileSideInMeters;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos, v2 Delta)
{
    WorldPos->P += Delta;
    CanonicalizeWorldPos(World, WorldPos);
}

// NOTE : This function always needs canonicalized position
internal b32
IsWorldPointEmptyUnchecked(world *World, world_position CanPos)
{
    b32 Result = false;

    tile_chunk *TileChunk = GetTileChunk(World, CanPos.AbsTileX, CanPos.AbsTileY);
    if(TileChunk)
    {
        Result = IsTileEmptyUnchecked(TileChunk, CanPos.AbsTileX, CanPos.AbsTileY);
    }

    return Result;
}

internal u32
ConvertMeterToTileCount(world *World, r32 ValueInMeter)
{
    u32 Result = 0;

    Result = (u32)(ValueInMeter/World->TileSideInMeters);

    return Result;
}

struct world_position_difference
{
    v2 P;
};
internal world_position_difference
WorldPositionDifferenceInMeter(world *World, world_position *A, world_position *B)
{
    world_position_difference Diff = {};

    i32 TileDiffX = B->AbsTileX - A->AbsTileX;
    i32 TileDiffY = B->AbsTileY - A->AbsTileY;

    Diff.P.X = TileDiffX*World->TileSideInMeters + (B->P.X - A->P.X);
    Diff.P.Y = TileDiffY*World->TileSideInMeters + (B->P.Y - A->P.Y);

    return Diff;
}

