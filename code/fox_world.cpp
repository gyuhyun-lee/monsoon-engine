#include "fox_world.h"

// TODO : Better hash function...?
#define WorldChunkHashValue(chunkX, chunkY, chunkZ) (19*chunkX + 32*chunkY + 7*chunkZ)

// Tile 0, 0, 0 matches to world chunk 0, 0, 0
internal world_p
TilePToWorldP(game_world *world, u32 tileX, u32 tileY, u32 tileZ)
{
	world_p result = {};

    r32 xInMeter = world->tileSizeInMeter*tileX + 0.5f*world->tileSizeInMeter;
    r32 yInMeter = world->tileSizeInMeter*tileY + 0.5f*world->tileSizeInMeter;
    r32 zInMeter = world->tileSizeInMeter*tileZ + 0.5f*world->tileSizeInMeter;

    result.chunkX = (u32)(xInMeter / world->chunkDim.x);
    result.chunkY = (u32)(yInMeter / world->chunkDim.y);
    result.chunkZ = (u32)(zInMeter / world->chunkDim.z);
    result.offset.x = xInMeter - result.chunkX*world->chunkDim.x;
    result.offset.y = yInMeter - result.chunkY*world->chunkDim.y;
    result.offset.z = zInMeter - result.chunkZ * world->chunkDim.z;

    return result;
}

internal void
ReCanonicalizeWorldP(game_world *world, world_p *p)
{
	i32 remainingChunkX = RoundDownr32Toi32(p->offset.x/world->chunkDim.x);
	i32 remainingChunkY = RoundDownr32Toi32(p->offset.y/world->chunkDim.y);
	i32 remainingChunkZ = RoundDownr32Toi32(p->offset.z/world->chunkDim.z);

	p->chunkX += remainingChunkX;
	p->offset.x -= remainingChunkX*world->chunkDim.x;

	p->chunkY += remainingChunkY;
	p->offset.y -= remainingChunkY*world->chunkDim.y;

	p->chunkZ += remainingChunkZ;
	p->offset.z -= remainingChunkZ*world->chunkDim.z;
}

internal world_chunk *
GetWorldChunkHash(game_world *world, i32 chunkX, i32 chunkY, i32 chunkZ, 
					memory_arena *memoryArena = 0)
{
	world_chunk *result = 0;

	// TODO : Better hash function...?
	u32 hashValue = WorldChunkHashValue(chunkX, chunkY, chunkZ);
	u32 hashKey = hashValue & (ArrayCount(world->chunkHashes) - 1);
	Assert(hashKey < ArrayCount(world->chunkHashes));

	world_chunk *worldChunk = world->chunkHashes + hashKey;

	do
	{
		if(worldChunk->chunkX == chunkX && 
			worldChunk->chunkY == chunkY &&
			worldChunk->chunkZ == chunkZ)
		{
			// Found the exact same hash
			result = worldChunk;
			break;
		}
		else if(worldChunk->seCount == 0)
		{
			// NOTE : This hash is empty so we can use this one
			result = worldChunk;
			result->chunkX = chunkX;
			result->chunkY = chunkY;
			result->chunkZ = chunkZ;
			break;
		}
		else
		{
			if(worldChunk->nextInHash)
			{
				worldChunk = worldChunk->nextInHash;
			}
		}
	}while(worldChunk->nextInHash);

	if(!result)
	{
		result = PushStruct(memoryArena, world_chunk);
		result->chunkX = chunkX;
		result->chunkY = chunkY;
		result->chunkZ = chunkZ;
		result->seCount = 0;

		worldChunk->nextInHash = result;
	}

	// TODO : For bug checking
	Assert(result);

	return result;
}

// This does not guarantee to find the world chunk.
internal world_chunk *
GetExistingWorldChunkHash(game_world *world, i32 chunkX, i32 chunkY, i32 chunkZ)
{
	world_chunk *result = 0;

	// TODO : Better hash function...?
	u32 hashValue = WorldChunkHashValue(chunkX, chunkY, chunkZ);
	u32 hashKey = hashValue & (ArrayCount(world->chunkHashes) - 1);
	Assert(hashKey < ArrayCount(world->chunkHashes));

	world_chunk *worldChunk = world->chunkHashes + hashKey;

	while(worldChunk)
	{
		if(worldChunk->chunkX == chunkX && 
			worldChunk->chunkY == chunkY &&
			worldChunk->chunkZ == chunkZ)
		{
			// Found the exact same hash
			result = worldChunk;
			break;
		}
		else
		{
			worldChunk = worldChunk->nextInHash;
		}
	}

	// TODO : For bug checking
	// Assert(result);

	return result;
}

internal void
PlaceEntityInsideWorldChunk(game_world *world, sleeping_entity *se)
{
    world_chunk *worldChunk = GetWorldChunkHash(world,
    											se->worldP.chunkX,
                                                se->worldP.chunkY,
                                                se->worldP.chunkZ);

    worldChunk->ses[worldChunk->seCount++] = se;
}

internal v3
SubstractTwoWorldPsInMeter(game_world *world, world_p p1, world_p p2)
{
	v3 result = {};

	i32 a = i32_min;
	i32 b = a - 10;

	// TODO : Still has problem with world wrapping!
	result.x = world->chunkDim.x*(p2.chunkX - p1.chunkX);
	result.y = world->chunkDim.y*(p2.chunkY - p1.chunkY);
	result.z = world->chunkDim.z*(p2.chunkZ - p1.chunkZ);
	result += p2.offset - p1.offset;

	return result;
}

// internal world_chunk *
// GetWorldChunkHash(game_world *world, u32 chunkX, u32 chunkY, u32 chunkZ)
// {
// 	world_chunk *result = 0;

// 	// TODO : Better hash function...?
// 	u32 hashValue = 19*chunkX + 32*chunkY + 7*chunkZ;
// 	u32 hashKey = hashValue & (ArrayCount(world->chunkHashes) - 1);
// 	Assert(hashKey < ArrayCount(world->chunkHashes));

// 	world_chunk *worldChunk = world->chunkHashes + hashKey;

// 	while(worldChunk)
// 	{
// 		if(worldChunk->chunkX == chunkX && 
// 			worldChunk->chunkY == chunkY &&
// 			worldChunk->chunkZ == chunkZ)
// 		{
// 			result = worldChunk;
// 		}
// 		else if(worldChunk->chunkX == r32_max)
// 		{
// 			result = worldChunk;
// 		}
// 		else
// 		{
// 			worldChunk = worldChunk->nextInHash;
// 		}
// 	}

// 	return result;
// }