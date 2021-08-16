#include "monsoon_sim_region.h"

internal void
TestWall(r32 WallX, r32 WallhalfDimY, v2 WallNormal, 
        r32 NewX, r32 OldX,  r32 NewY, r32 OldY,
        r32 *tMin, b32 *Hit, v2 *HitWallNormal)
{
    r32 dX = NewX - OldX;

    if(dX != 0.0f)
    {
        r32 tTest = (WallX - OldX)/dX;

        if(tTest >= 0.0f && tTest < 1.0f)
        {
            if(tTest < *tMin)
            {
                r32 tY = OldY + tTest*(NewY - OldY);
                if(tY > -WallhalfDimY && tY < WallhalfDimY)
                {
                    *Hit = true;
                    *tMin = tTest;
                    *HitWallNormal = WallNormal;
                }
            }
        }
    }
}

internal void
MoveEntity(sim_region *simRegion, sim_entity *entity, v3 ddP, r32 speed, r32 dtPerFrame)
{
    r32 ddPLength = LengthSquare(ddP);
    if(ddPLength > 1.0f)
    {
        ddP *= 1.0f/SquareRoot2(ddPLength);
    }

    ddP *= speed;
    ddP -= 8.0f*entity->dP;
    /*
     * NOTE :
     * Position = 0.5f*a*dt*dt + previous frame v * dt + previous frame p
     * Velocity = a*dt + v;
    */   
    v3 EntityDelta = 0.5f*Square(dtPerFrame)*ddP + 
                    dtPerFrame*entity->dP;
    v3 RemainingEntityDelta = EntityDelta;
    entity->dP = dtPerFrame*ddP + entity->dP;
    r32 DistanceLeftSquare = LengthSquare(RemainingEntityDelta);

    for(u32 CollisionIteration = 0;
        CollisionIteration < 4;
        ++CollisionIteration)
    {
        if(DistanceLeftSquare > 0.0f)
        {
            v3 NewEntityPos = entity->p + RemainingEntityDelta;

            r32 tMin = 1.0f;
            v2 HitWallNormal = {};
            b32 Hit = false;

#if 1
            for(u32 EntityIndex = 0;
                EntityIndex < simRegion->entityCount;
                ++EntityIndex)
            {
                sim_entity *testEntity = simRegion->entities + EntityIndex;
                if(testEntity != entity)
                {
                    v3 MinkowskihalfDim = 0.5f*(testEntity->dim + entity->dim);
                    v3 testEntityRelNewP = NewEntityPos - testEntity->p;
                    v3 testEntityRelOldP = entity->p - testEntity->p;

                    // TODO : Collision is off for z value
                    if(testEntityRelNewP.z >= -MinkowskihalfDim.z &&
                        testEntityRelNewP.z < MinkowskihalfDim.z)
                    {

                        // NOTE : Test against left wall
                        TestWall(-MinkowskihalfDim.x, MinkowskihalfDim.y, V2(-1, 0), 
                                testEntityRelNewP.x, testEntityRelOldP.x, testEntityRelNewP.y, testEntityRelOldP.y, 
                                &tMin, &Hit, &HitWallNormal);

                        // NOTE : Test against right wall
                        TestWall(MinkowskihalfDim.x, MinkowskihalfDim.y, V2(1, 0), 
                                testEntityRelNewP.x, testEntityRelOldP.x, testEntityRelNewP.y, testEntityRelOldP.y, 
                                &tMin, &Hit, &HitWallNormal);

                        // NOTE : Test against upper wall
                        TestWall(MinkowskihalfDim.y, MinkowskihalfDim.x, V2(0, -1), 
                                testEntityRelNewP.y, testEntityRelOldP.y, testEntityRelNewP.x, testEntityRelOldP.x, 
                                &tMin, &Hit, &HitWallNormal);

                        // NOTE : Test against bottom wall
                        TestWall(-MinkowskihalfDim.y, MinkowskihalfDim.x, V2(0, 1), 
                                testEntityRelNewP.y, testEntityRelOldP.y, testEntityRelNewP.x, testEntityRelOldP.x, 
                                &tMin, &Hit, &HitWallNormal);
                    }
                }
            }
#endif

            v3 EntityDeltaForThisIteration = RemainingEntityDelta;
            v3 EntityDeltaLeftForThisIteration = V3(0, 0, 0);
            v3 OldRemainingEntityDelta = RemainingEntityDelta;

            if(Hit)
            {
                // TODO : What to do with this epsilon?
                r32 tEpsilon = 0.0001f;

                EntityDeltaForThisIteration = (tMin - tEpsilon)*RemainingEntityDelta;

                r32 EntityDeltaForThisIterationLengthSquare = LengthSquare(EntityDeltaForThisIteration);
                if(EntityDeltaForThisIterationLengthSquare > DistanceLeftSquare)
                {
                    EntityDeltaForThisIteration *= SquareRoot2(DistanceLeftSquare/EntityDeltaForThisIterationLengthSquare);
                }

                v3 EntityDeltaLeftForThisIteration = RemainingEntityDelta - EntityDeltaForThisIteration;

                // TODO : Another z value hack in here!
                entity->dP = entity->dP - 1.0f*Dot(entity->dP, V3(HitWallNormal, 0.0f))*V3(HitWallNormal, 0.0f);

                RemainingEntityDelta = EntityDeltaLeftForThisIteration - 1.0f*Dot(EntityDeltaLeftForThisIteration, V3(HitWallNormal, 0))*V3(HitWallNormal, 0);
            }

            entity->p += EntityDeltaForThisIteration;

            // TODO : Clean this code!
            r32 LengthSquareMovedForThisIteration = LengthSquare(EntityDeltaForThisIteration);
            r32 LengthSquareCanceledByWall = LengthSquare(EntityDeltaLeftForThisIteration - RemainingEntityDelta);
            DistanceLeftSquare -= LengthSquareMovedForThisIteration + LengthSquareCanceledByWall;
        }
    }

    //CanonicalizeWorldPos(world, &entity->worldP, EntityDelta);
}


internal void
StartSimRegion(sim_region *simRegion, world *world, world_position center, v3 dim, v3 MaxEntityDelta)
{
    simRegion->entityCount = 0;
    simRegion->center = center;
    simRegion->halfDim = 0.5f*dim + MaxEntityDelta;

    // IMPORTANT : NOTE : Sim region should NOT be bigger than the maximum movable length of
    // the base(centered, most likely the player) entity. If the sim region somehow 
    // was not bigger than the max length, it will clear all the entity located at 
    // the sim region at the end of sim region.
    world_position simRegionLeftBottom = simRegion->center;
    CanonicalizeWorldPos(world, &simRegionLeftBottom, -simRegion->halfDim);

    world_position simRegionUpperRight = simRegion->center;
    CanonicalizeWorldPos(world, &simRegionUpperRight, simRegion->halfDim);

    u32 MinchunkX = simRegionLeftBottom.chunkX;
    u32 MinchunkY = simRegionLeftBottom.chunkY;
    u32 MinchunkZ = simRegionLeftBottom.chunkZ;
    u32 OnePastMaxchunkX = simRegionUpperRight.chunkX + 1;
    u32 OnePastMaxchunkY = simRegionUpperRight.chunkY + 1;
    u32 OnePastMaxchunkZ = simRegionUpperRight.chunkZ + 1;

    u32 ChunkDiffX = OnePastMaxchunkX - MinchunkX;
    u32 ChunkDiffY = OnePastMaxchunkY - MinchunkY;
    u32 ChunkDiffZ = OnePastMaxchunkZ - MinchunkZ;

    for(u32 chunkZIndex = 0;
        chunkZIndex < ChunkDiffZ;
        ++chunkZIndex)
    {
        u32 chunkZ = MinchunkZ + chunkZIndex;
        for(u32 chunkYIndex = 0;
            chunkYIndex < ChunkDiffY;
            ++chunkYIndex)
        {
            u32 chunkY = MinchunkY + chunkYIndex;
            for(u32 chunkXIndex = 0;
                chunkXIndex < ChunkDiffX;
                ++chunkXIndex)
            {
                u32 chunkX = MinchunkX + chunkXIndex;
                world_chunk *worldChunk = GetWorldChunk(world, chunkX, chunkY, chunkZ);

                if(worldChunk)
                {
                    low_entity_block *entityBlock = &worldChunk->entityBlock;
                    while(entityBlock)
                    {               
                        for(u32 EntityIndex = 0;
                            EntityIndex < entityBlock->entityCount;
                            ++EntityIndex)
                        {
                            if(chunkXIndex == 1 &&
                                chunkYIndex == 2 &&
                                chunkZIndex == 0)
                            {
                                int a = 1;
                            }
                            low_entity *entity = entityBlock->entities[EntityIndex];
                            sim_entity *simEntity = simRegion->entities + simRegion->entityCount++;

                            Assert(simRegion->entityCount <= ArrayCount(simRegion->entities));

                            v3 simRegionRelP = WorldPositionDifferenceInMeter(world, &entity->worldP, &simRegion->center);
                            simEntity->p = simRegionRelP;

                            simEntity->dP = entity->dP;
                            simEntity->dim = entity->dim;
                            simEntity->type = entity->type;

                            simEntity->lowEntity = entity;
                        }

                        entityBlock = entityBlock->next;
                    }
                }
            }
        }
    }
}

internal void
ClearAllentityBlocksInworldChunk(world_chunk *worldChunk)
{
    // NOTE : Set the entity count in all entity blocks to 0
    low_entity_block *entityBlock = &worldChunk->entityBlock;
    while(entityBlock)
    {               
        entityBlock->entityCount = 0;

        entityBlock = entityBlock->next;
    }
}

internal void
ClearAllentityBlocksInsimRegion(sim_region *simRegion, world *world)
{
    world_position simRegionLeftBottom = simRegion->center;
    CanonicalizeWorldPos(world, &simRegionLeftBottom, -simRegion->halfDim);

    world_position simRegionUpperRight = simRegion->center;
    CanonicalizeWorldPos(world, &simRegionUpperRight, simRegion->halfDim);

    u32 MinchunkX = simRegionLeftBottom.chunkX;
    u32 MinchunkY = simRegionLeftBottom.chunkY;
    u32 MinchunkZ = simRegionLeftBottom.chunkZ;
    u32 OnePastMaxchunkX = simRegionUpperRight.chunkX + 1;
    u32 OnePastMaxchunkY = simRegionUpperRight.chunkY + 1;
    u32 OnePastMaxchunkZ = simRegionUpperRight.chunkZ + 1;

    u32 ChunkDiffX = OnePastMaxchunkX - MinchunkX;
    u32 ChunkDiffY = OnePastMaxchunkY - MinchunkY;
    u32 ChunkDiffZ = OnePastMaxchunkZ - MinchunkZ;

    for(u32 chunkZIndex = 0;
        chunkZIndex < ChunkDiffZ;
        ++chunkZIndex)
    {
        u32 chunkZ = MinchunkZ + chunkZIndex;
        for(u32 chunkYIndex = 0;
            chunkYIndex < ChunkDiffY;
            ++chunkYIndex)
        {
            u32 chunkY = MinchunkY + chunkYIndex;
            for(u32 chunkXIndex = 0;
                chunkXIndex < ChunkDiffX;
                ++chunkXIndex)
            {
                u32 chunkX = MinchunkX + chunkXIndex;
                world_chunk *worldChunk = GetWorldChunk(world, chunkX, chunkY, chunkZ);
                if(worldChunk)
                {
                    ClearAllentityBlocksInworldChunk(worldChunk);
                }
            }
        }
    }
}

internal void
EndSimRegion(world *world, memory_arena *arena, sim_region *simRegion)
{
    ClearAllentityBlocksInsimRegion(simRegion, world);
    for(u32 simEntityIndex = 0;
        simEntityIndex < simRegion->entityCount;
        ++simEntityIndex)
    {
        sim_entity *simEntity = simRegion->entities + simEntityIndex;
        low_entity *entity = simEntity->lowEntity;
        world_position newWorldPos = simRegion->center;
        CanonicalizeWorldPos(world, &newWorldPos, simEntity->p);

        if(newWorldPos.chunkZ == UINT_MAX)
        {
            int a = 1;
        }
        entity->worldP = newWorldPos;
        entity->dP = simEntity->dP;

        world_chunk *worldChunk = GetWorldChunk(world, newWorldPos.chunkX, newWorldPos.chunkY, newWorldPos.chunkZ, 1);
        PutEntityInsideWorldChunk(worldChunk, arena, entity);
    }
}

