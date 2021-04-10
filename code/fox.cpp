/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

/* 
    TODO

    win32
    - xinput for gamepad support
    -? Handle oldInput in platform layer

    game
    - entity inside world chunk
    - entity hashing
    - pairwise collision rule hashing
*/

#include <stdio.h>
#include "fox.h"
#include "fox_raycast_2d.cpp"
#include "fox_world.cpp"
#include "fox_active_region.cpp"
#include "fox_render.cpp"

#pragma pack(push, 1)
struct bitmap_file_info
{
    i16 signature;
    i32 fileSize;
    i32 reserved1;
    i32 dataOffset;

    i32 size;
    i32 width;
    i32 height;
    i16 planes;
    i16 bitsPerPixel;
    i32 compression;
    i32 imageSize;
    i32 horizontalResolution;
    i32 verticalResolution;
    i32 colorsUsed;
    i32 importantColors;

    i8 redMask;
    i8 greenMask;
    i8 blueMask;
    i8 reserved2;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBitmap(debug_platform_read_file *PlatformReadFile, char *filePath)
{
    loaded_bitmap result = {};

    bitmap_file_info *info = (bitmap_file_info *)PlatformReadFile(filePath);
    if(info)
    {
        Assert(info->signature == Encode2Bytes("BM", false));
        Assert(info->compression == 3 && info->bitsPerPixel == 32);

        // TODO : Color mask support
        result.width = info->width;
        result.height = info->height;
        result.pitch = 4*info->width;
        result.data = (u8 *)info + info->dataOffset;
    }

    return result;
}

internal r32
FollowCircularly(r32 targetradius, 
                r32 followerradius, 
                r32 radiusLimit)
{
    r32 clockwiseDistance = Modular(followerradius - targetradius, Tau32);
    r32 counterClockwiseDistance = Tau32 - clockwiseDistance;

    if(clockwiseDistance >= radiusLimit && counterClockwiseDistance >= radiusLimit)
    {
        if(clockwiseDistance >= counterClockwiseDistance)
            followerradius += 0.1f; 
        else
            followerradius -= 0.1f;
    }

    return followerradius;
}
internal void
AddNullEntity(game_state *gameState)
{
    sleeping_entity *se = gameState->ses + gameState->seCount++;
}

internal sleeping_entity *
AddSleepingEntity(game_state *gameState, u32 tileX, u32 tileY, u32 tileZ, v3 halfDim, entity_type type, collision_group *collisionGroup = 0)
{
    u32 ID = gameState->seCount++;
    sleeping_entity *se = gameState->ses + ID;
    se->worldP = TilePToWorldP(&gameState->world, tileX, tileY, tileZ);

    PlaceEntityInsideWorldChunk(&gameState->world, se);

    se->ae.ID = ID;
    se->ae.type = type;
    se->ae.halfDim = halfDim;
    se->ae.collisionGroup = collisionGroup;

    return se;
}

internal sleeping_entity *
AddPlayerEntity(game_state *gameState, u32 tileX, u32 tileY, u32 tileZ, v3 halfDim, collision_group *collisionGroup = 0)
{
    sleeping_entity *se = AddSleepingEntity(gameState, tileX, tileY, tileZ, halfDim, entity_type_player, collisionGroup);

    se->ae.flags = entity_flag_collidable | entity_flag_movable;
    if(gameState->cameraFollowingEntityID == 0)
    {
        gameState->cameraFollowingEntityID = se->ae.ID;
    }
    return se;
}

internal sleeping_entity *
AddEnemyEntity(game_state *gameState, u32 tileX, u32 tileY, u32 tileZ, v3 halfDim, collision_group *collisionGroup = 0)
{
    sleeping_entity *se = AddSleepingEntity(gameState, tileX, tileY, tileZ, halfDim, entity_type_enemy, collisionGroup);

    se->ae.flags = entity_flag_collidable|entity_flag_damageOnCollide;
    
    se->ae.hp = 3;

    return se;
}

internal sleeping_entity *
AddFamiliarEntity(game_state *gameState, u32 tileX, u32 tileY, u32 tileZ, v3 halfDim, collision_group *collisionGroup = 0)
{
    sleeping_entity *se = AddSleepingEntity(gameState, tileX, tileY, tileZ, halfDim, entity_type_familiar, collisionGroup);
    return se;
}
internal sleeping_entity *
AddBlockEntity(game_state *gameState, u32 tileX, u32 tileY, u32 tileZ, v3 halfDim, collision_group *collisionGroup = 0)
{
    sleeping_entity *se = AddSleepingEntity(gameState, tileX, tileY, tileZ, halfDim, entity_type_block, collisionGroup);
    se->ae.flags = entity_flag_collidable;
    se->ae.hp = 1;
    return se;
}

internal collision_group *
AllocateCollisionGroup(memory_arena *arena, u32 polygonCount)
{
    collision_group *result = 0;
    result = (collision_group *)PushStruct(arena, collision_group);
    result->polygonCount = polygonCount;
    result->polygons = (collision_polygon *)PushArray(arena, collision_polygon, result->polygonCount);

    return result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	game_state *gameState = (game_state *)gameMemory->permanentStorage;
	transient_state *tranState = (transient_state *)gameMemory->transientStorage;
    u8 *permanentStorageFreeMemoryStart = (u8 *)gameMemory->permanentStorage + sizeof(game_state);
    u8 *transientStorageFreeMemoryStart = (u8 *)gameMemory->transientStorage + sizeof(transient_state);

	if(!gameState->isInitialized)
	{
        ChangeRandomSeed();
        gameState->gameArena = StartMemoryArena(permanentStorageFreeMemoryStart, Megabytes(64));

        gameState->world.tileSizeInMeter = 1.6f;
        gameState->world.chunkDim = gameState->world.tileSizeInMeter*V3(20, 20, 20);

        gameState->bitmap = DEBUGLoadBitmap(platformAPI->PlatformReadFile, "../fox/data/sample.bmp");

        v3 playerHalfDim = V3(0.6f, 1.6f, 0.2f);
        gameState->playerCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->playerCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->playerCollisionGroup->polygons[0].halfDim = playerHalfDim;

        v3 enemyHalfDim = V3(2.0f, 2.0f, 0.2f);
        gameState->enemyCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->enemyCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->enemyCollisionGroup->polygons[0].halfDim = enemyHalfDim;

        v3 jumperHalfDim = V3(50, 50, 20);
        gameState->jumperCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->jumperCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->jumperCollisionGroup->polygons[0].halfDim = jumperHalfDim;

        v3 blockHalfDim = V3(100, 5, 2.19f);
        gameState->blockCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->blockCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->blockCollisionGroup->polygons[0].halfDim = blockHalfDim;

        v3 verticalBlockHalfDim = V3(5, 100, 2.19);
        gameState->verticalBlockCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->verticalBlockCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->verticalBlockCollisionGroup->polygons[0].halfDim = verticalBlockHalfDim;

        v3 moonHalfDim = V3(10, 10, 2.19f);
        gameState->moonCollisionGroup = AllocateCollisionGroup(&gameState->gameArena, 1);
        gameState->moonCollisionGroup->polygons[0].offset = V3(0, 0, 0);
        gameState->moonCollisionGroup->polygons[0].halfDim = moonHalfDim;


        v3 centerP = V3(10, 10, 0);
        AddNullEntity(gameState);
        AddPlayerEntity(gameState, 21, 50, 0, playerHalfDim, gameState->playerCollisionGroup);
        AddBlockEntity(gameState, 21, 30, 0, blockHalfDim, gameState->blockCollisionGroup);

        gameState->camera.worldP = GetSleepingEntity(gameState, gameState->cameraFollowingEntityID)->worldP; 
        gameState->camera.halfDim = V3(50.8f, 100.0f, 20.0f);

		gameState->isInitialized = true;
	}
	if(!tranState->isInitialized)
	{
		tranState->isInitialized = true;
	}

    r32 mToPixel = 0.15f;
    v2 bufferHalfDimInMeter = mToPixel*0.5f*V2((r32)gameBuffer->width, (r32)gameBuffer->height);

    controlled_player player_ = {};
    controlled_player *player = &gameState->player;
    sleeping_entity *playerEntity = GetSleepingEntity(gameState, gameState->cameraFollowingEntityID);

    v2 mouseP01 = gameInput->controllers[0].rightStick;
                    
    player->ddp = {};
    player->angle = {};
    player->attackKeyPressed = false;
    player->dashKeyPressed = false;
    player->speed = V2(700, 700);
    player->drag = 10.0f;

    if(GetButtonDown(gameInput, 0, key_action_right))
    {
        // SetdtScale(gameTimeManager, 0.4f);
    }

    player->ddp.xy = GetLeftStickValue(gameInput, 0);

    if(gameState->dashDuration > 0.0f)
    {
        // player->dashKeyPressed = true;
        // player->ddp = Normalize(V3(mouseWorldP, 0) - playerEntity->ae.p);
        // player->speed = (1/gameTimeManager->dtScale)*V2(1000.0f, 1000.0f);
        // player->damageOnCollideTime = 0.2f;
        // player->drag = 10.0f;
        // player->drag = 15.0f;

        SetEntityFlag(&playerEntity->ae.flags, entity_flag_damageOnCollide);
    }
    else
    {
        if(GetButtonTriggered(gameInput, oldInput, 0, key_action_right))
        {
            playerEntity->ae.v = V3(0, 0, 0);

            gameState->maxDashDuration = 0.13f;
            gameState->dashDuration = gameState->maxDashDuration;
            player->dashKeyPressed = true;
            player->ddp.xy = Normalize(mouseP01);
            player->damageOnCollideTime = 0.2f;
            player->drag = 10.0f;
            SetEntityFlag(&playerEntity->ae.flags, entity_flag_damageOnCollide);
        }
    }

    if(GetButtonDown(gameInput, 0, key_action_down))
    {
        // if(CheckUpperMostStatusStack(&player->statusStack, status_move))
        {
            // if(playerEntity->ae.jumpRemaining-- > 0)
            if(IsEntityFlagSet(playerEntity->ae.flags, entity_flag_ysupported))
            {
                player->ddp.y += 1.f;
                UnsetEntityFlag(&playerEntity->ae.flags, entity_flag_ysupported);
                // playerEntity->ae.v.y = 0.0f;

            }
        }
        // player->ddp.y += 1.0f;
    }


    active_region activeRegion = {};
    activeRegion.center = playerEntity->worldP;
    activeRegion.halfDim = V3(200, 200, 0);
    StartActiveRegion(gameState, &gameState->world, &activeRegion);

    // NOTE : Start Rendering
    render_group renderGroup = {};
    renderGroup.renderMemory = StartTemporaryMemory(&gameState->gameArena, Megabytes(4));
    renderGroup.camera = gameState->camera;
    renderGroup.mToPixel = mToPixel;

    renderGroup.camera.halfDim = V3(bufferHalfDimInMeter, 20.0f); // TODO : camera half dim must be buffer half dim for now. Change this to support any camera half dim? (For resolution, maybe?) 
    ClearBuffer(gameBuffer, 0xff000019);

    for(u32 aeIndex = 0;
        aeIndex < activeRegion.aeCount;
        ++aeIndex)
    {
        active_entity *ae = activeRegion.aes + aeIndex;

        v3 ddp = {};
        v3 ddpUnaffectedBySpeed = {};
        movement_info movementInfo = DefaultMovementInfo();

        v2 xAxis = V2(Cos(ae->angle), Sin(ae->angle));
        v2 yAxis = V2(Cos(ae->angle + Rad(90)), Sin(ae->angle + Rad(90)));
        v2 facingXAxis = V2(Cos(ae->facingAngle), Sin(ae->facingAngle));
        v2 facingYAxis = V2(Cos(ae->facingAngle + Rad(90)), Sin(ae->facingAngle + Rad(90)));

        switch(ae->type)
        {
            case entity_type_player:
            {
                ae->facingAngle = atan2f(mouseP01.y, mouseP01.x);
                movementInfo.shouldddpNormalized = true;
                movementInfo.speed = player->speed;
                movementInfo.drag = player->drag;
                ddp = player->ddp;

                if(player->attackKeyPressed)
                {
                    ddpUnaffectedBySpeed += -100.f*V3(Cos(ae->facingAngle), Sin(ae->facingAngle), 0.0f);
                }

                if(player->dashKeyPressed)
                {
                    v3 p = RaycastToAllAes(&activeRegion, ae, ae->facingAngle);
                    // ae->p = p;
                    gameState->dashP = p;
                    gameState->dashAngle = ae->facingAngle;
                }

                if(gameState->dashDuration > 0.0f)
                {
                    PushRectWithAngle(gameBuffer, &renderGroup, gameState->dashP, V3(2.0f*bufferHalfDimInMeter.x, 0.6f, 0.0f),
                            AwesomeColor(255.0f*gameState->dashDuration/gameState->maxDashDuration), gameState->dashAngle); 
                }
                // ddp = player->ddp.y * V3(Cos(ae->angle), Sin(ae->angle), 0.0f);
                // ae->angle += player->angle;


            }break;
            case entity_type_familiar:
            {

            }break;
            case entity_type_block:
            {

            }break;

            case entity_type_jumper:
            {
            }break;
            case entity_type_moon:
            {
                ae->p = ae->center + ae->radius*V3(Cos(ae->centerBasedAngle), Sin(ae->centerBasedAngle), 0.0f);
                ae->centerBasedAngle += 0.1f;
            }break;
            case entity_type_enemy :
            {
                // ddp = -V3(Cos(ae->centerBasedAngle), Sin(ae->centerBasedAngle), 0);
            }break;
        }

        if(IsEntityFlagSet(ae->flags, entity_flag_movable))
        {
        }

        switch(ae->type)
        {
            case entity_type_player:
            {
                // gameState->camera.center = ae->p;
                particle_emitter *particleEmitter = &ae->particleEmitter;
                particleEmitter->angle = ae->angle + Pi32;
                particleEmitter->angleRange = Rad(10);
                particleEmitter->speed = 5.f;
                particleEmitter->speedRange = 1.0001f;
                particleEmitter->life = 1.5f;
                particleEmitter->lifeRange = 0.5f;
                particleEmitter->halfDim = V3(1, 1, 0);
                particleEmitter->halfDimRange = V3(0, 0, 0);

                for(u32 particlesIndex = 0;
                    particlesIndex < ArrayCount(particleEmitter->particles); 
                    ++particlesIndex)
                {
                    particle *particle = particleEmitter->particles + particlesIndex;

                    if(particle->lifeTime > 0.0f)
                    {
                        v4 color = AwesomeColor();
                        color.a = 255.0f*(particle->lifeTime / particle->maxLifeTime);

                        r32 particleHalfLength = Length(particleEmitter->halfDim);
                        r32 particleRad = Pi32*5.0f/4;
                        PushRect(gameBuffer, &renderGroup,
                                particle->p, particleEmitter->halfDim, color,
                                V2(Cos(particleRad), Sin(particleRad)), V2(Cos(particleRad+Rad(90)), Sin(particleRad+Rad(90))));

                    }
                    else
                    {
                        r32 particleAngle = RandomBetween(particleEmitter->angle - particleEmitter->angleRange,
                                                        particleEmitter->angle + particleEmitter->angleRange);
                        r32 particleSpeed = RandomBetween(particleEmitter->speed - particleEmitter->speedRange,
                                                        particleEmitter->speed + particleEmitter->speedRange);
                        
                        particle->p = ae->p;
                        particle->v = V3();
                        // particle->v = particleSpeed*V3(Cos(particleAngle), Sin(particleAngle), 0.0f);
                        particle->a = particleSpeed*V3(-1, -1, 0.0f);
                        particle->maxLifeTime = RandomBetween(particleEmitter->life - particleEmitter->lifeRange,
                                                            particleEmitter->life + particleEmitter->lifeRange);
                        particle->lifeTime = particle->maxLifeTime;
                    }
                }

                v4 color = V4(255, 255, 255, 255);

                PushRect(gameBuffer, &renderGroup,
                        ae->p, ae->halfDim, color, xAxis, yAxis);

                if(player->attackKeyPressed)
                {
                    PushRect(gameBuffer, &renderGroup, ae->p + bufferHalfDimInMeter.x*V3(facingXAxis, 0), V3(bufferHalfDimInMeter.x, 1.0f, 0.0f),
                            AwesomeColor(), facingXAxis, facingYAxis); 
                }
            }break;
            case entity_type_familiar:
            {
            }break;
            case entity_type_block:
            {
                v4 color = V4(255, 255, 255, 255);
                PushRect(gameBuffer, &renderGroup, ae->p, ae->halfDim, color, 
                            xAxis, yAxis);
                // PushBitmap(gameBuffer, &renderGroup, &gameState->bitmap, ae->p, ae->halfDim, color, 
                //             xAxis, yAxis);
            }break;
            case entity_type_jumper:
            {
                v4 color = V4(50, 10, 50, 255);

                PushRectOutline(gameBuffer, &renderGroup, 
                            ae->p, ae->halfDim, color);
            }break;
            case entity_type_moon:
            {
                v4 color = V4(200, 200, 150, 255);
                PushRect(gameBuffer, &renderGroup,
                        ae->p, ae->halfDim, color);
            }break;
            case entity_type_enemy:
            {
                if(ae->hp > 0)
                {
                    v4 color = V4(100, 255, 200, 255);
                    PushRect(gameBuffer, &renderGroup,
                            ae->p, ae->halfDim, color);
                }
            }break;
            default:
            {
                v4 color = V4(100, 255, 200, 255);
                PushRect(gameBuffer, &renderGroup,
                        ae->p, ae->halfDim, color);
            }break;
        }       

        // for(u32 statusIndex = ae->statusStack.statusCount;
        //     statusIndex > 0;
        //     --statusIndex)
        // {
        //     entity_status *status = ae->statusStack.statuses + statusIndex - 1;
        //     if(status->effectDuration < 0)
        //     {
        //         --ae->statusStack.statusCount;
        //     }
        //     else
        //     {
        //         status->effectDuration -= Currentdt(gameTimeManager);
        //     }
        // }

#if 0
        if(ae->collisionGroup)
        {
            for(u32 polygonIndex = 0;
                polygonIndex < ae->collisionGroup->polygonCount;
                ++polygonIndex)
            {
                collision_polygon *polygon = ae->collisionGroup->polygons + polygonIndex;
                v3 polygonP = ae->p + polygon->offset;
                PushRectOutline(gameBuffer, &renderGroup, 
                            polygonP, polygon->halfDim, V4(50.f, 120.f, 255.0f, 255.0f));
            }
        }
#endif
    }

    RenderAllEntries(&renderGroup, gameBuffer);
    ResetStatusStack(&player->statusStack);

    EndTemporaryMemory(&renderGroup.renderMemory);
    EndActiveRegion(gameState, &gameState->world, &activeRegion);
}

#if 0
internal void
OuputSineWave(game_state *gameState, game_audio_buffer *gameAudioBuffer, i16 toneVolume, r32 toneHz)
{
    i32 wavePeriod = gameAudioBuffer->samplesPerSecond/toneHz;

    i16 *destLeft = gameAudioBuffer->samples[0];
    i16 *destRight = gameAudioBuffer->samples[1];

    // NOTE : We are always filling one frame worth of samples
    // samplesPerSecond / targetFramesPerSec
    for(u32 sampleIndex = 0;
        sampleIndex < gameAudioBuffer->sampleCount;
        ++sampleIndex)
    {
        r32 sineValue = Sin(gameState->tSine);
        i16 sampleValue = (i16)(sineValue * toneVolume);

        *destLeft++ += (i16)sampleValue;
        *destRight++ += (i16)sampleValue;

#if 0
        gameState->tSine += (Tau32)/(r32)wavePeriod;
        if(gameState->tSine > Tau32)
        {
            gameState->tSine -= Tau32;
        }
#endif 

        ++gameAudioBuffer->runningSampleIndex;
    }   
}
#endif

internal void
ClearAudioBuffer(i16 *leftSamples, i16 *rightSamples, u32 sampleCount)
{
    for(u32 sampleIndex = 0;
        sampleIndex < sampleCount;
        ++sampleIndex)
    {
        *leftSamples++ = 0;
        *rightSamples++ = 0;
    }
}

extern "C" GAME_UPDATE_AUDIO_BUFFER(GameUpdateAudioBuffer)
{
#if 0
	game_state *gameState = (game_state *)gameMemory->permanentStorage;
	transient_state *tranState = (transient_state *)gameMemory->transientStorage;

    i16 *destLeft = gameAudioBuffer->samples[0];
    i16 *destRight = gameAudioBuffer->samples[1];
    for(u32 sampleIndex = 0;
        sampleIndex < gameAudioBuffer->sampleCokm;
        ++sampleIndex)
    {
        *destLeft++ = 0;
        *destRight++ = 0;
    }
    destLeft = gameAudioBuffer->samples[0];
    destRight = gameAudioBuffer->samples[1];

    for(u32 sampleIndex = 0;
        sampleIndex < gameAudioBuffer->sampleCount;
        ++sampleIndex)
    {
#if 0
        for(u32 playingSoundIndex = 0;
            playingSoundIndex < 1;
            ++playingSoundIndex)
#endif
        {
            playing_sound *playingSound = &gameState->playingSound;
            i16 *sourceBuffer = (i16 *)playingSound->loadedWav->data;
            i16 sourceLeft = sourceBuffer[2*playingSound->runningSampleIndex];
            i16 sourceRight = sourceBuffer[2*playingSound->runningSampleIndex + 1];

            *destLeft++ += sourceLeft;
            *destRight++ += sourceRight;

            ++playingSound->runningSampleIndex;
        }
    }

    //OuputSineWave(gameState, gameAudioBuffer, 1000, 256.0f);
#endif
}
