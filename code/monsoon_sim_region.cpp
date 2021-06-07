#include "monsoon_sim_region.h"

internal void
StartSimRegion(world *World, sim_region *SimRegion, world_position Center, v2 HalfDim, v2 MaxEntityDelta)
{
    SimRegion->EntityCount = 0;
    SimRegion->Center = Center;
    SimRegion->HalfDim = HalfDim + MaxEntityDelta;

    // IMPORTANT : NOTE : Sim region should NOT be bigger than the maximum movable length of
    // the base(centered, most likely the player) entity. If the sim region somehow 
    // was not bigger than the max length, it will clear all the entity located at 
    // the sim region at the end of sim region.
    world_position SimRegionLeftBottom = SimRegion->Center;
    CanonicalizeWorldPos(World, &SimRegionLeftBottom, -SimRegion->HalfDim);

    world_position SimRegionUpperRight = SimRegion->Center;
    CanonicalizeWorldPos(World, &SimRegionUpperRight, SimRegion->HalfDim);

    u32 MinChunkX = SimRegionLeftBottom.ChunkX;
    u32 MinChunkY = SimRegionLeftBottom.ChunkY;
    u32 OnePastMaxChunkX = SimRegionUpperRight.ChunkX + 1;
    u32 OnePastMaxChunkY = SimRegionUpperRight.ChunkY + 1;
    u32 MinChunkZ = 0;
    u32 OnePastMaxChunkZ = 1;

    u32 ChunkDiffX = OnePastMaxChunkX - MinChunkX;
    u32 ChunkDiffY = OnePastMaxChunkY - MinChunkY;
    u32 ChunkDiffZ = OnePastMaxChunkZ - MinChunkZ;

    for(u32 ChunkZIndex = 0;
        ChunkZIndex < ChunkDiffZ;
        ++ChunkZIndex)
    {
        u32 ChunkZ = MinChunkZ + ChunkZIndex;
        for(u32 ChunkYIndex = 0;
            ChunkYIndex < ChunkDiffY;
            ++ChunkYIndex)
        {
            u32 ChunkY = MinChunkY + ChunkYIndex;
            for(u32 ChunkXIndex = 0;
                ChunkXIndex < ChunkDiffX;
                ++ChunkXIndex)
            {
                u32 ChunkX = MinChunkX + ChunkXIndex;
                world_chunk *WorldChunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                low_entity_block *EntityBlock = &WorldChunk->EntityBlock;
                while(EntityBlock)
                {               
                    for(u32 EntityIndex = 0;
                        EntityIndex < EntityBlock->EntityCount;
                        ++EntityIndex)
                    {
                        low_entity *Entity = EntityBlock->Entities[EntityIndex];
                        sim_entity *SimEntity = SimRegion->Entities + SimRegion->EntityCount++;

                        v2 SimRegionRelP = WorldPositionDifferenceInMeter(World, &Entity->WorldP, &SimRegion->Center);
                        SimEntity->P = SimRegionRelP;

                        SimEntity->dP = Entity->dP;
                        SimEntity->Dim = Entity->Dim;
                        SimEntity->Type = Entity->Type;

                        SimEntity->LowEntity = Entity;
                    }

                    EntityBlock = EntityBlock->Next;
                }
            }
        }
    }
}

internal void
ClearAllEntityBlocksInWorldChunk(world_chunk *WorldChunk)
{
    // NOTE : Set the entity count in all entity blocks to 0
    low_entity_block *EntityBlock = &WorldChunk->EntityBlock;
    while(EntityBlock)
    {               
        EntityBlock->EntityCount = 0;

        EntityBlock = EntityBlock->Next;
    }
}

internal void
ClearAllEntityBlocksInSimRegion(sim_region *SimRegion, world *World)
{
    world_position SimRegionLeftBottom = SimRegion->Center;
    CanonicalizeWorldPos(World, &SimRegionLeftBottom, -SimRegion->HalfDim);

    world_position SimRegionUpperRight = SimRegion->Center;
    CanonicalizeWorldPos(World, &SimRegionUpperRight, SimRegion->HalfDim);

    u32 MinChunkX = SimRegionLeftBottom.ChunkX;
    u32 MinChunkY = SimRegionLeftBottom.ChunkY;
    u32 OnePastMaxChunkX = SimRegionUpperRight.ChunkX + 1;
    u32 OnePastMaxChunkY = SimRegionUpperRight.ChunkY + 1;
    u32 MinChunkZ = 0;
    u32 OnePastMaxChunkZ = 1;

    u32 ChunkDiffX = OnePastMaxChunkX - MinChunkX;
    u32 ChunkDiffY = OnePastMaxChunkY - MinChunkY;
    u32 ChunkDiffZ = OnePastMaxChunkZ - MinChunkZ;

    for(u32 ChunkZIndex = 0;
        ChunkZIndex < ChunkDiffZ;
        ++ChunkZIndex)
    {
        u32 ChunkZ = MinChunkZ + ChunkZIndex;
        for(u32 ChunkYIndex = 0;
            ChunkYIndex < ChunkDiffY;
            ++ChunkYIndex)
        {
            u32 ChunkY = MinChunkY + ChunkYIndex;
            for(u32 ChunkXIndex = 0;
                ChunkXIndex < ChunkDiffX;
                ++ChunkXIndex)
            {
                u32 ChunkX = MinChunkX + ChunkXIndex;
                world_chunk *WorldChunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                ClearAllEntityBlocksInWorldChunk(WorldChunk);
            }
        }
    }
}

internal void
PutEntityInsideWorldChunk(world_chunk *WorldChunk, memory_arena *Arena, low_entity *Entity)
{
    low_entity_block *EntityBlock = 0;
    low_entity_block *Search = &WorldChunk->EntityBlock;
    while(!EntityBlock)
    {
        if(Search->EntityCount < ArrayCount(WorldChunk->EntityBlock.Entities))
        {
            EntityBlock = Search;
        }
        else
        {
            if(Search->Next)
            {
                Search = Search->Next;
            }
            else
            {
                EntityBlock = PushStruct(Arena, low_entity_block);
                Search->Next = EntityBlock;
            }
        }
    }

    EntityBlock->Entities[EntityBlock->EntityCount++] = Entity; 
}

internal void
EndSimRegion(world *World, memory_arena *Arena, sim_region *SimRegion)
{
    ClearAllEntityBlocksInSimRegion(SimRegion, World);
    for(u32 SimEntityIndex = 0;
        SimEntityIndex < SimRegion->EntityCount;
        ++SimEntityIndex)
    {
         sim_entity *SimEntity = SimRegion->Entities + SimEntityIndex;
        low_entity *Entity = SimEntity->LowEntity;
        world_position NewWorldPos = SimRegion->Center;
        CanonicalizeWorldPos(World, &NewWorldPos, SimEntity->P);
        Entity->WorldP = NewWorldPos;
        Entity->dP = SimEntity->dP;

        world_chunk *WorldChunk = GetWorldChunk(World, NewWorldPos.ChunkX, NewWorldPos.ChunkY, NewWorldPos.ChunkZ);
        PutEntityInsideWorldChunk(WorldChunk, Arena, Entity);
    }
}

