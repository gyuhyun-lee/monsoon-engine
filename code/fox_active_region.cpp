#include "fox_active_region.h"

inline movement_info
DefaultMovementInfo()
{
	movement_info result = {};
	result.shouldddpNormalized = false;
	result.speed = V2(10.0f, 10.0f);

	return result;
}

inline void
SetEntityFlag(u8 *flags, u8 flag)
{
    *flags |= flag;
}

inline void
UnsetEntityFlag(u8 *flags, u8 flag)
{
	u8 unsetFlag = ~flag;
	*flags &= unsetFlag;
	// flags
}

inline b32
IsEntityFlagSet(u8 flags, u8 flag)
{
	b32 result = false;

	if(flags & flag)
	{
		result = true;
	}

	return result;
}
internal void
StartActiveRegion(game_state *gameState, game_world *world, active_region *activeRegion)
{
	// TODO : min and max chunk might not be 100% accurate for now.
	// Once more precise chunks are needed, I'll work on this more.
	u32 activeRegionChunkSizeX = (u32)(activeRegion->halfDim.x/world->chunkDim.x);
	u32 activeRegionChunkSizeY = (u32)(activeRegion->halfDim.y/world->chunkDim.y);
	u32 activeRegionChunkSizeZ = (u32)(activeRegion->halfDim.z/world->chunkDim.z);

	// TODO : prevent min & max overflow
	i32 minChunkX = activeRegion->center.chunkX - activeRegionChunkSizeX;
	i32 minChunkY = activeRegion->center.chunkY - activeRegionChunkSizeY;
	i32 minChunkZ = activeRegion->center.chunkZ - activeRegionChunkSizeZ;

	i32 maxChunkX = activeRegion->center.chunkX + activeRegionChunkSizeX;
	i32 maxChunkY = activeRegion->center.chunkY + activeRegionChunkSizeY;
	i32 maxChunkZ = activeRegion->center.chunkZ + activeRegionChunkSizeZ;

	for(i32 chunkZ = minChunkZ;
		chunkZ <= maxChunkZ;
		++chunkZ)
	{
		for(i32 chunkY = minChunkY;
			chunkY <= maxChunkY;
			++chunkY)
		{
			for(i32 chunkX = minChunkX;
				chunkX <= maxChunkX;
				++chunkX)
			{
				world_chunk *worldChunk = GetExistingWorldChunkHash(world, chunkX, chunkY, chunkZ);

				if(worldChunk)
				{
					for(u32 entityIndex = 0;
						entityIndex < worldChunk->seCount;
						++entityIndex)
					{
						sleeping_entity *se = worldChunk->ses[entityIndex];
						active_entity *ae = activeRegion->aes + activeRegion->aeCount++;
						*ae = se->ae; // TODO : Copying all of the data is a overhead.

						ae->p = SubstractTwoWorldPsInMeter(world, activeRegion->center, se->worldP);

						if(ae->ID == gameState->cameraFollowingEntityID)
						{
							// gameState->camera.worldP = se->worldP;
							// gameState->camera.p = ae->p;
						}
						// ae->p = WorldPToActiveRegionP();
						}

					worldChunk->seCount = 0;
				}
			}
		}
	}

	gameState->camera.p = SubstractTwoWorldPsInMeter(world, activeRegion->center, gameState->camera.worldP);
}


struct test_wall
{
	r32 startY;
	r32 endY;
	r32 wallX;
	v3 normal;

	r32 testX;
	r32 entityDeltaY;
};

internal r32
TestWall(test_wall *wall)
{
	// NOTE : This function assumes that the relatvie position was already calculated
	// so the testP is 0, 0, 0
	r32 result = 1.0f;

	// TODO : 
	// if(Inner(wall->normal, V3(wall->testX, wall->entityDeltaY, 0.0f)) < 0.0f)
	{
		r32 t = (wall->wallX) / wall->testX;

		r32 newy = t*wall->entityDeltaY;

		if(t >= 0.0f &&
			newy >= wall->startY && newy < wall->endY)
		{
			result = t;
		}
	}

	return result;
}

internal b32
ShouldStopOnCollide(active_entity *a, active_entity *b)
{
	b32 result = true;

	if(a->type == entity_type_player && b->type == entity_type_jumper)
	{
		result = false;
	}

	if(a->type == entity_type_player && b->type == entity_type_enemy)
	{
		result = false;
	}

	return result;
}

internal void
HandleCollision(active_entity *actor, active_entity *target)
{
	if(actor->type == entity_type_player && target->type == entity_type_block)
	{
		SetEntityFlag(&actor->flags, entity_flag_ysupported);
	}

	if(actor->type == entity_type_player && target->type == entity_type_enemy)
	{
		if(IsEntityFlagSet(actor->flags, entity_flag_damageOnCollide))
		{
		}
	}
}

internal void
MoveEntityWithddp(active_region *activeRegion, active_entity *ae, v3 ddp, v3 ddpUnaffectedBySpeed, movement_info movementInfo, r32 fixeddt, game_state *gameState)
{ 
	r32 lengthOfddp = Length(ddp);
	b32 isInputNull = false;
	if(lengthOfddp == 0.0f)
	{
		isInputNull = true;
	}

	if(movementInfo.shouldddpNormalized && !isInputNull)
	{
		ddp = Normalize(ddp);
	}

	ddp.x *= movementInfo.speed.x;
	ddp.y *= movementInfo.speed.y;
	// if(!IsEntityFlagSet(ae->flags, entity_flag_ysupported))
	{
		// Gravity
		if(gameState->dashDuration < 0.0f)
		{
			ddp -= V3(0, 900.8f, 0.0f);
		}
	}
	ddp -= movementInfo.drag*ae->v;
	
	ddp += ddpUnaffectedBySpeed;
	
	v3 newV = ae->v + ddp*fixeddt;
    v3 entityDelta = newV*fixeddt + 0.5f*ddp*fixeddt*fixeddt;  // NOTE : For this frame!
    v3 newP = ae->p + entityDelta;
    r32 distanceLimit = Length(entityDelta);

    if(IsEntityFlagSet(ae->flags, entity_flag_collidable))
    {
		for(u32 collisionIter = 0;
			collisionIter < 4;
			++collisionIter)
		{
			if(distanceLimit > 0.0f)
			{
			    r32 tMin = 1.0f;
			    test_wall hitWall = {};
			    u32 hitWallIndex = 0; // TODO : This is mainly for detecing floor... Should be a better way to handle floor!

			    active_entity *hitae = 0;
			    b32 didHitWall = false;

			    for(u32 aeIndex = 0;
			    	aeIndex < activeRegion->aeCount;
			    	++aeIndex)
			    {
			    	active_entity *testae = activeRegion->aes + aeIndex;

			    	if(ae != testae && IsEntityFlagSet(testae->flags, entity_flag_collidable))
			    	{
		    			for(u32 polygonIndex = 0;
			                polygonIndex < ae->collisionGroup->polygonCount;
			                ++polygonIndex)
			            {
				            collision_polygon *polygon = ae->collisionGroup->polygons + polygonIndex;

			            	for(u32 testPolygonIndex = 0;
				                testPolygonIndex < testae->collisionGroup->polygonCount;
				                ++testPolygonIndex)
				            {
				                collision_polygon *testPolygon = testae->collisionGroup->polygons + testPolygonIndex;

						    	v3 minkowskiHalfDim = testPolygon->halfDim + polygon->halfDim;
					    		v3 relP = (testae->p+testPolygon->offset) - (ae->p+polygon->offset);
					    		test_wall walls[4] = 
					    		{
									// startY, 						endY, 						wallX, 						normal, 	testX, 			entityDeltaY
					    			{relP.y - minkowskiHalfDim.y, relP.y + minkowskiHalfDim.y, relP.x + minkowskiHalfDim.x, V3(1, 0, 0), entityDelta.x, entityDelta.y}, //right
					    			{relP.x - minkowskiHalfDim.x, relP.x + minkowskiHalfDim.x, relP.y + minkowskiHalfDim.y, V3(0, 1, 0), entityDelta.y, entityDelta.x}, //up
					    			{relP.y - minkowskiHalfDim.y, relP.y + minkowskiHalfDim.y, relP.x - minkowskiHalfDim.x, V3(-1, 0, 0), entityDelta.x, entityDelta.y}, //left
					    			{relP.x - minkowskiHalfDim.x, relP.x + minkowskiHalfDim.x, relP.y - minkowskiHalfDim.y, V3(0, -1, 0), entityDelta.y, entityDelta.x} // down
					    		};

					    		for(u32 wallIndex = 0;
					    			wallIndex < ArrayCount(walls);
					    			++wallIndex)
					    		{
					    			test_wall *wall = walls + wallIndex;
									r32 t = TestWall(wall);
									if(tMin > t)
									{
										if(ShouldStopOnCollide(ae, testae))
										{
											tMin = t;
											didHitWall = true;
											hitWall = *wall;
											hitWallIndex = wallIndex;
										}

										hitae = testae;
									}
					    		}
					    	}
			            }
			    	}
			    }

			    v3 thisIterEntityDelta = tMin*entityDelta;
			    r32 thisIterEntityDeltaLength = Length(thisIterEntityDelta);

			    if(distanceLimit < thisIterEntityDeltaLength)
			    {
			    	thisIterEntityDelta = distanceLimit*(thisIterEntityDelta/thisIterEntityDeltaLength);
			    	thisIterEntityDeltaLength = distanceLimit;
			    }

			    ae->p += thisIterEntityDelta;
				distanceLimit -= thisIterEntityDeltaLength;

			    if(didHitWall)
			    {
			    	v3 remainingEntityDelta = entityDelta - thisIterEntityDelta;

			    	r32 reflectionFactor = 1.0f; // 1 : glide, 2 : reflect
			    	newV = newV + 1.0f * Inner(-hitWall.normal, newV) * hitWall.normal;
					entityDelta = remainingEntityDelta + reflectionFactor*Inner(-hitWall.normal, remainingEntityDelta)*hitWall.normal;
			    	if(ae->type == entity_type_player)
			    	{
			    		// distanceLimit = 0.0f;
			    		// ae->v = V3();
			    		// canKeepMoving = false;
			    	}
			    }

			    ae->v = newV;

			    if(hitae)
			    {
			    	HandleCollision(ae, hitae);
			    }

			}
		}
    }
    else
    {
    	// TODO : This route is for the entities that does have flag_move, but does not have
    	// flag_collide. Better way to do this?
    	ae->p += entityDelta;
    	ae->v = newV;
    }
}


internal void
RaycastToAllAesResultOnly(active_region *activeRegion, active_entity *ae, r32 angle)
{
	for(u32 aeIndex = 0;
    	aeIndex < activeRegion->aeCount;
    	++aeIndex)
    {
    	active_entity *testae = activeRegion->aes + aeIndex;

    	if(ae != testae)
    	{
			if(RaycastToNonRotatedSquareResultOnly(ae->p.xy, angle, testae->p.xy, testae->halfDim.xy))
			{
				if(ae->type == entity_type_player && testae->type == entity_type_enemy)
				{
					testae->hp -= 1;
				}
			}
    	}
    }
}

internal v3
RaycastToAllAes(active_region *activeRegion, active_entity *ae, r32 angle)
{
	v3 resultP = ae->p;
	r32 distanceSquare = 100000000000.0f;

	for(u32 aeIndex = 0;
    	aeIndex < activeRegion->aeCount;
    	++aeIndex)
    {
    	active_entity *testae = activeRegion->aes + aeIndex;

    	if(ae != testae)
    	{
    		raycast_2d_result result = RaycastToNonRotatedSquare(ae->p.xy, angle, testae->p.xy, testae->halfDim.xy);
    		if(result.collided)
    		{
	    		r32 distanceBetween = LengthSq(result.p - ae->p.xy);
	    		if(distanceSquare > distanceBetween)
	    		{
	    			resultP = V3(result.p, 0);
	    		}
	    	}
    	}
    }

    return resultP;
}

internal void
EndActiveRegion(game_state *gameState, game_world *world, active_region *activeRegion)
{
	for(u32 aeIndex = 0;
		aeIndex < activeRegion->aeCount;
		++aeIndex)
	{
		active_entity *ae = activeRegion->aes + aeIndex;
		sleeping_entity *se = GetSleepingEntity(gameState, ae->ID);

		world_p newWorldP = activeRegion->center;
		newWorldP.offset += ae->p;
		ReCanonicalizeWorldP(&gameState->world, &newWorldP);

		se->ae = *ae;// TODO : Again, not thrilled about this whole copy overhead
		se->worldP = newWorldP;

		PlaceEntityInsideWorldChunk(world, se);
	}
}

// internal void
// MoveEntity(active_entity *activeEntity, )
// {
// }
