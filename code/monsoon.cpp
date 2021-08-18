/*
 * Monsoon Engine is a 3D engine built by Joon Lee.
 */
#include "monsoon_platform_independent.h"
#include "monsoon_intrinsic.h"
#include "monsoon_random.h"
#include "monsoon_world.h"

#if MONSOON_DEBUG
debug_game_cycle_counter *DEBUGGlobalCycleCounter;
#endif

#include "monsoon_world.cpp"
#include "monsoon_sim_region.cpp"
#include "monsoon_render.cpp"

#include "monsoon.h"

internal void
MakeCheckerBoard(pixel_buffer_32 *LOD, v4 color)
{
    color.rgb *= color.a;
    u32 c = (u32)((RoundR32ToInt32(color.a * 255.0f) << 24) |
                    (RoundR32ToInt32(color.r * 255.0f) << 16) |
                    (RoundR32ToInt32(color.g * 255.0f) << 8) |
                    (RoundR32ToInt32(color.b * 255.0f) << 0));

    b32 checkBoardIsEmpty = false;
    u8 *row = (u8 *)LOD->memory;
    u32 checkBoardY = 0;
    for(u32 y = 0;
        y < LOD->height;
        ++y)
    {
        u32 checkBoardX = 0;
        u32 *pixel = (u32 *)row;
        for(u32 x = 0;
            x < LOD->width;
            ++x)
        {
            *pixel++ = checkBoardIsEmpty ? 0xff111111 : c;

            checkBoardX++;
            if(checkBoardX == 16)
            {
                checkBoardX = 0;
                checkBoardIsEmpty = !checkBoardIsEmpty;
            }
        }
        checkBoardY++;
        if(checkBoardY == 16)
        {
            checkBoardY = 0;
            checkBoardIsEmpty = !checkBoardIsEmpty;
        }

        row += LOD->pitch;
    }

}
#if 1
// NOTE : This is a complete hack for now, as the bitmap image we have is 2D, but we should
// think it as a 3D.
// NOTE : So here's what we are going to do with these normal maps. X and Y is same as
// 2D, and the positive Z value = direction facing towards us.
// Therefore, it's actually a Y value that decides whether we should sample from 
// the upper environment map or bottom environmentmap, and X & Z value decide what pixel
// we should sample from those enviroment map.
internal pixel_buffer_32
MakeSphereNormalMap(memory_arena *arena, i32 width, i32 height, r32 roughness)
{
    pixel_buffer_32 buffer = MakeEmptyPixelBuffer32(arena, width, height);

    u8 *row = (u8 *)buffer.memory;
    for(i32 y = 0;
        y < buffer.height;
        ++y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = 0;
            x < buffer.width;
            ++x)
        {
            v2 uv = V2((r32)x/(r32)(buffer.width - 1), (r32)y/(r32)(buffer.height));
            // NOTE : Put inside -1 to 1 space
            r32 normalX = 2.0f*uv.x - 1.0f;
            r32 normalY = 2.0f*uv.y - 1.0f;
            r32 normalZ = 0.0f;

            // NOTE : equation of a circle is x^2+y^2+z^2 = r^2
            r32 rawZ = 1.0f - (normalX*normalX + normalY*normalY);
            if(rawZ >= 0.0f)
            {
                // NOTE : As we can see here, z cannot be negative value
                // which is we want, because we should not be seeing negative z value(which is the opposite side of our viewing direction)
                normalZ = SquareRoot2(rawZ);
            }
            else
            {
                normalX = 0.f;
                normalY = 0.f;
                normalZ = -1.f;
            }

            // NOTE : Again, put this to 0 to 255 range.
            u32 resultX = RoundR32ToUInt32(255.0f*0.5f*(normalX + 1.0f));
            u32 resultY = RoundR32ToUInt32(255.0f*0.5f*(normalY + 1.0f));
            u32 resultZ = RoundR32ToUInt32(255.0f*0.5f*(normalZ + 1.0f));
            *pixel++ = (resultX << 16 |
                        resultY << 8 |
                        resultZ << 0 |
                        (RoundR32ToUInt32(255.0f*roughness) << 24));
        }

        row += buffer.pitch;
    }

    return buffer;
}
#endif




/*
 * TODO : 
 *  - Think about how I want to handle the Z value
 *  - ground chunk pixel buffer creation
*/

#pragma pack(push, 1)
struct debug_bmp_file_header
{
    u16 FileHeader;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 pixelOffset;
    u32 HeaderSize;
    u32 width;
    u32 height;
    u16 ColorPlaneCount;
    u16 BitsPerPixel;
    u32 Compression;

    u32 ImageSize;
    u32 PixelsInMeterX;
    u32 PixelsInMeterY;
    u32 Colors;
    u32 ImportantColorCount;
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 AlphaMask;
};
#pragma pack(pop)

internal pixel_buffer_32
DEBUGLoadBMP(debug_read_entire_file *ReadEntireFile, char *FileName, v2 alignment = V2(0, 0))
{
    // TODO : Currently only supports bmp with compression = 3
    pixel_buffer_32 result = {};

    debug_platform_read_file_result File = ReadEntireFile(FileName);
    debug_bmp_file_header *Header = (debug_bmp_file_header *)File.memory;
    Assert(Header->Compression == 3);
    u32 RedShift = FindLeastSignificantSetBit(Header->RedMask);
    u32 GreenShift = FindLeastSignificantSetBit(Header->GreenMask);
    u32 BlueShift = FindLeastSignificantSetBit(Header->BlueMask);
    u32 AlphaShift = FindLeastSignificantSetBit(Header->AlphaMask);

    result.width = Header->width;
    result.height = Header->height;
    result.widthOverHeight = (r32)result.width/(r32)result.height;
    result.bytesPerPixel = Header->BitsPerPixel/8;
    result.pitch = result.width * result.bytesPerPixel;
    result.alignPercentage.x = alignment.x/(r32)result.width;
    result.alignPercentage.y = alignment.y/(r32)result.height;

    result.memory = (u32 *)((u8 *)Header + Header->pixelOffset);

    u8 *Sourcerow = (u8 *)result.memory;
    for(u32 Y = 0;
        Y < Header->height;
        ++Y)
    {
        u32 *SourcePixel = (u32 *)Sourcerow;
        for(u32 X = 0;
            X < Header->width;
            ++X)
        {
            u32 A = ((*SourcePixel >> AlphaShift) & 0x000000ff);
            r32 R32A = A/255.0f;

            r32 R = (r32)((*SourcePixel >> RedShift) & 0x000000ff)*R32A;
            r32 G = (r32)((*SourcePixel >> GreenShift) & 0x000000ff)*R32A;
            r32 B = (r32)((*SourcePixel >> BlueShift) & 0x000000ff)*R32A;

            *SourcePixel++ = (RoundR32ToUInt32(A) << 24 |
                                RoundR32ToUInt32(R) << 16 |
                                RoundR32ToUInt32(G) << 8 |
                                RoundR32ToUInt32(B) << 0);
        }

        Sourcerow += result.pitch;
    }
    
    return result;
}

internal low_entity *
AddLowEntity(game_state *state, entity_type Type, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, 
            v3 Dim)
{
    Assert(state->entityCount < ArrayCount(state->entities));

    low_entity *entity = state->entities + state->entityCount++;

    entity->worldP.p = state->world.tileSideInMeters*V3(AbsTileX, AbsTileY, AbsTileZ) + 
                        0.5f*state->world.tileSideInMeters*V3(1, 1, 1) - 
                        0.5f*state->world.chunkDim;
    //entity->worldP.P.Z = 0; // TODO : Properly handle Z
    CanonicalizeWorldPos(&state->world, &entity->worldP);

    entity->dim = Dim;
    entity->type = Type;

    world_chunk *worldChunk = GetWorldChunk(&state->world, 
                                            entity->worldP.chunkX, 
                                            entity->worldP.chunkY, 
                                            entity->worldP.chunkZ, 1);
    PutEntityInsideWorldChunk(worldChunk, &state->worldArena, entity);

    return entity;
}

internal void
AddPlayerentity(game_state *state, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, v3 Dim)
{
    state->player = AddLowEntity(state, EntityType_Player, AbsTileX, AbsTileY, AbsTileZ, Dim);
}

// TODO : This should go away!
#define TILE_COUNT_X 10
#define TILE_COUNT_Y 10
#define TILE_COUNT_Z 10

extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
#if MONSOON_DEBUG
    DEBUGGlobalCycleCounter = Memory->DEBUGCycleCounters;
#endif
    BeginCycleCounter(GameUpdateAndRender);

    Assert(sizeof(game_state) <= Memory->permanentStorageSize);

    game_state *state = (game_state *)Memory->permanentStorage;

    if(!state->isInitialized)
    {
        state->worldArena = StartMemoryArena((u8 *)Memory->permanentStorage + sizeof(*state), Megabytes(256));

        world *world = &state->world;
        world->tileSideInMeters = 2.0f;
        world->chunkDim.x = TILE_COUNT_X*world->tileSideInMeters;
        world->chunkDim.y = TILE_COUNT_Y*world->tileSideInMeters;
        world->chunkDim.z = TILE_COUNT_Z*world->tileSideInMeters;
        InitializeWorld(world);

        state->cameraPos.chunkX = 0;
        state->cameraPos.chunkY = 0;
        state->cameraPos.chunkZ = 0;
        state->cameraPos.p.x = 0;
        state->cameraPos.p.y = 0;

        AddPlayerentity(state, 6, 4, 1, 
                        V3(0.5f*world->tileSideInMeters, 0.9*world->tileSideInMeters, 1));

        random_series Series = Seed(123);
        // TODO : Proper map construction!
        // NOTE : Construction starts with bottom map
        b32 LeftShouldBeOpened = false;
        b32 BottomShouldBeOpened = false;
        b32 RightShouldBeOpened = false;
        b32 UpShouldBeOpened = true;
        
        // TODO : Massive change of map construction is needed!
        // so that I don't have to think about this tile & chunk stuff all the time..
#if 1
        for(u32 z = 0;
            z < 3*TILE_COUNT_Z;
            z += TILE_COUNT_Z/2)
#endif
        {
            u32 LeftBottomTileX = 0;
            u32 LeftBottomTileY = 0;
            
            for(u32 ScreenIndex = 0;
                ScreenIndex < 10;
                ++ScreenIndex)
            {
                for(u32 Y = 0;
                    Y < TILE_COUNT_Y;
                    ++Y)
                {
                    u32 row = LeftBottomTileY + Y;
                    for(u32 X = 0;
                        X < TILE_COUNT_X;
                        ++X)
                    {
                        u32 column = LeftBottomTileX + X;
                        b32 shouldAddWall = true;

                        if(LeftShouldBeOpened)
                        {
                            if(Y == TILE_COUNT_Y/2 && X == 0)
                            {
                                shouldAddWall = false;
                            }
                        }
                        if(RightShouldBeOpened)
                        {
                            if(Y == TILE_COUNT_Y/2 && X == TILE_COUNT_X-1)
                            {
                                shouldAddWall = false;
                            }
                        }
                        if(BottomShouldBeOpened)
                        {
                            if(X == TILE_COUNT_X/2 && Y == 0)
                            {
                                shouldAddWall = false;
                            }
                        }
                        if(UpShouldBeOpened)
                        {
                            if(X == TILE_COUNT_X/2 && Y == TILE_COUNT_Y-1)
                            {
                                shouldAddWall = false;
                            }
                        }

                        if(!(X == 0 || X == TILE_COUNT_X - 1 || Y == 0 || Y == TILE_COUNT_Y - 1))
                        {
                            shouldAddWall = false;
                        }

                        if(shouldAddWall)
                        {
                            if(z == 10)
                            {
                                int breakhere = 1;
                            }
                            AddLowEntity(state, EntityType_Wall, column, row, z, 
                                        world->tileSideInMeters*V3(1, 1, 1));
                        }
                    }
                }

                if(RightShouldBeOpened)
                {
                    LeftBottomTileX += TILE_COUNT_X;
                    LeftShouldBeOpened = true;
                    u32 RandomNumber = GetNextRandomNumberInSeries(&Series);
                    if(RandomNumber % 2)
                    {
                        // NOTE : map with up & left opened
                        UpShouldBeOpened = true;
                        BottomShouldBeOpened = false;
                        RightShouldBeOpened = false;
                    }
                    else
                    {
                        // NOTE : map with right & left opened
                        RightShouldBeOpened = true;
                        BottomShouldBeOpened = false;
                        UpShouldBeOpened = false;
                    }
                }
                else if(UpShouldBeOpened)
                {
                    LeftBottomTileY += TILE_COUNT_Y;
                    BottomShouldBeOpened = true;
                    u32 RandomNumber = GetNextRandomNumberInSeries(&Series);
                    if(RandomNumber % 2)
                    {
                        // NOTE : map with up & bottom opened
                        UpShouldBeOpened = true;
                        LeftShouldBeOpened = false;
                        RightShouldBeOpened = false;
                    }
                    else
                    {
                        // NOTE : map with bottom & right opened
                        RightShouldBeOpened = true;
                        LeftShouldBeOpened = false;
                        UpShouldBeOpened = false;
                    }
                }
            }
        }

        state->treeBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/tree00.bmp");

        state->sampleBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/sample.bmp");

        state->headBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_head.bmp",
                                        V2(0, 34));
        state->torsoBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_torso.bmp",
                                       V2(0, 34));
        state->capeBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_cape.bmp",
                                    V2(0, 34));

        state->rockBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock00.bmp",
                                        V2(35, 40));
        state->rockBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock01.bmp",
                                        V2(35, 40));
        state->rockBMP[2] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock02.bmp",
                                        V2(35, 40));
        state->rockBMP[3] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock03.bmp",
                                        V2(35, 40));

        state->groundBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground00.bmp",
                                            V2(136, 60));
        state->groundBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground01.bmp",
                                            V2(136, 60));
        state->groundBMP[2] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground02.bmp",
                                            V2(136, 60));
        state->groundBMP[3] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground03.bmp",
                                            V2(136, 60));

        state->grassBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass00.bmp",
                                            V2(82, 65));
        state->grassBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass01.bmp",
                                            V2(82, 65));

        state->renderArena = StartMemoryArena((u8 *)Memory->transientStorage, Megabytes(512));
        state->backgroundBuffer.width = 800;
        state->backgroundBuffer.height = 600;
        state->backgroundBuffer.bytesPerPixel = 4;
        state->backgroundBuffer.pitch = state->backgroundBuffer.width*state->backgroundBuffer.bytesPerPixel;
        state->backgroundBuffer.memory = (u32 *)PushSize(&state->renderArena, state->backgroundBuffer.pitch*state->backgroundBuffer.height);

        environment_map *topEnvMap = state->envMaps + 2;
        environment_map *middleEnvMap = state->envMaps + 1;
        environment_map *bottomEnvMap = state->envMaps;
        topEnvMap->LOD = MakeEmptyPixelBuffer32(&state->renderArena, 512, 256);
        middleEnvMap->LOD = MakeEmptyPixelBuffer32(&state->renderArena, topEnvMap->LOD.width, topEnvMap->LOD.height);
        bottomEnvMap->LOD = MakeEmptyPixelBuffer32(&state->renderArena, topEnvMap->LOD.width, topEnvMap->LOD.height);

        MakeCheckerBoard(&topEnvMap->LOD, V4(1, 0, 0, 1));
        MakeCheckerBoard(&middleEnvMap->LOD, V4(0, 1, 0, 1));
        MakeCheckerBoard(&bottomEnvMap->LOD, V4(0, 0, 1, 1));

        state->testDiffuse = MakeEmptyPixelBuffer32(&state->renderArena, 256, 256, V2(0, 0), 
                                                    V4(0.2f, 0.2f, 0.2f, 1));
        state->sphereNormalMap = MakeSphereNormalMap(&state->renderArena, state->testDiffuse.width, state->testDiffuse.height, 1.0f);
        //MakeBackground(state, &state->backgroundBuffer);
        //
        state->finalBuffer = MakeEmptyPixelBuffer32(&state->renderArena, offscreenBuffer->width, offscreenBuffer->height);

        state->isInitialized = true;
    }
    world *world = &state->world;

    game_input Input = {}; // NOTE : Sum of all the raw inputs
    for(u32 ControllerIndex = 0;
        ControllerIndex < RawInput->controllerCount;
        ++ControllerIndex)
    {
        game_controller *rawController = RawInput->controllers + ControllerIndex;
        if(rawController->isAnalog)
        {
            // NOTE : This is a controller
            if(rawController->leftStickX > 0.3f || rawController->DPadRight)
            {
                Input.moveRight = true;
            }
            if(rawController->leftStickX < -0.3f || rawController->DPadLeft)
            {
                Input.moveLeft = true;
            }
            if(rawController->leftStickY > 0.3f || rawController->DPadUp)
            {
                Input.moveUp = true;
            }
            if(rawController->leftStickY < -0.3f || rawController->DPadDown)
            {
                Input.moveDown = true;
            }
        }
        else
        {
            // NOTE : This is keyboard
            if(rawController->moveUp)
            {
                Input.moveUp = rawController->moveUp||Input.moveUp;
            }
            if(rawController->moveDown)
            {
                Input.moveDown = rawController->moveDown||Input.moveDown;
            }
            if(rawController->moveLeft)
            {
                Input.moveLeft = rawController->moveLeft||Input.moveLeft;
            }
            if(rawController->moveRight)
            {
                Input.moveRight = rawController->moveRight||Input.moveRight;
            }
        }

        if(rawController->AButton)
        {
            Input.actionRight = rawController->AButton||Input.actionRight;
        }
        if(rawController->BButton)
        {
            Input.actionDown = rawController->BButton||Input.actionDown;
        }
        if(rawController->YButton)
        {
            Input.actionUp = rawController->YButton||Input.actionUp;
        }
    }

    v3 ddPlayer = {};
    if(Input.moveRight)
    {
        ddPlayer.x += 1.0f;
    }
    if(Input.moveLeft)
    {
        ddPlayer.x -= 1.0f;
    }
    if(Input.moveUp)
    {
        ddPlayer.y += 1.0f;
    }
    if(Input.moveDown)
    {
        ddPlayer.y -= 1.0f;
    }
    if(Input.actionDown)
    {
        ddPlayer.z -= 1.0f;
    }
    if(Input.actionUp)
    {
        ddPlayer.z += 1.0f;
    }

    // TODO : This is also wrong for 3D
    // TODO : This is wrong when the player is using controller.
    if(ddPlayer.x != 0.0f && ddPlayer.y != 0.0f)
    {
        ddPlayer *= 0.70710678118f;
    }

    r32 playerSpeed = 50.0f;
    if(Input.actionRight)
    {
        playerSpeed = 150.f;
    }

    state->cameraPos = state->player->worldP;
    render_group renderGroup = {};
    StartRenderGroup(&renderGroup, &state->renderArena, Megabytes(256), offscreenBuffer->width, offscreenBuffer->height);

    sim_region simRegion = {};
    // TODO : Accurate max entity delta for the sim region!
    StartSimRegion(&simRegion, world, state->cameraPos, 
                    V3(renderGroup.gameCamera.projectedMonitorDim, world->chunkDim.z), 
                    V3(2, 2, 1));

    for(u32 entityIndex = 0;
        entityIndex < simRegion.entityCount;
        ++entityIndex)
    {
        sim_entity *entity = simRegion.entities + entityIndex;

        switch(entity->type)
        {
            case EntityType_Player:
            {
                MoveEntity(&simRegion, entity, ddPlayer, playerSpeed, dtPerFrame);
            }break;
        }
    }

#if 0
    MakeCheckerBoard(&state->envMaps[0].LOD, V4(0, 0, 1, 1));
    MakeCheckerBoard(&state->envMaps[1].LOD, V4(0, 1, 0, 1));
    MakeCheckerBoard(&state->envMaps[2].LOD, V4(1, 0, 0, 1));
#endif

    for(u32 entityIndex = 0;
        entityIndex < simRegion.entityCount;
        ++entityIndex)
    {
        sim_entity *entity = simRegion.entities + entityIndex;

        r32 zDiff = Abs(entity->p.z);

        r32 cameraZ = 5.0f;
        r32 alphaBasedOnZ = 1.0f;
        r32 clipPlaneZ = 0.5f*world->chunkDim.z;
        if(zDiff > clipPlaneZ)
        {
            alphaBasedOnZ = Clamp01(1.0f - (zDiff - clipPlaneZ)/clipPlaneZ);
        }

        switch(entity->type)
        {
            case EntityType_Wall:
            {
                PushBMP(&renderGroup, &state->treeBMP, entity->p, entity->dim,
                        V4(1, 1, 1, alphaBasedOnZ));
            }break;

            case EntityType_Player: 
            {
                r32 cos = Cos(state->theta);
                r32 sin = Sin(state->theta);
#if 0
                v2 xAxis = V2(cos, sin);
                v2 yAxis = V2(-sin, cos);

#else

                v2 xAxis = V2(1, 0);
                v2 yAxis = V2(0, 1);
#endif

                PushRect(&renderGroup, entity->p, entity->dim, V4(1, 1, 0, 1), xAxis, yAxis);
                PushBMP(&renderGroup, &state->headBMP, entity->p,entity->dim, V4(1, 1, 1, alphaBasedOnZ),
                        xAxis, yAxis);
                PushBMP(&renderGroup, &state->torsoBMP, entity->p,entity->dim, V4(1, 1, 1, alphaBasedOnZ),
                        xAxis, yAxis);
                PushBMP(&renderGroup, &state->capeBMP, entity->p,entity->dim, V4(1, 1, 1, alphaBasedOnZ),
                        xAxis, yAxis);
                /*
                PushBMP(&renderGroup, &state->headBMP, entity->p + 1.0f*V3(Cos(state->theta), 0, 0), bitmapDimInMeter,
                        V2(1, 0), V2(0, 1),
                        &state->sphereNormalMap, state->envMaps);
                        */
            }break;
        }
    }
    ClearPixelBuffer32(&state->finalBuffer, V4(0.5f, 0.5f, 0.5f, 1));

#if 0
    // NOTE : Backgrounds are not affected by the MetersToPixels value. 
    // Is this intentional or not?
    v2 BackgroundP = 0.7f*renderGroup.BufferHalfDim;
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP, V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP + V2(state->backgroundBuffer.width, 0), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP - V2(state->backgroundBuffer.width, 0), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP + V2(0, state->backgroundBuffer.height), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP - V2(0, state->backgroundBuffer.height), V2(0, 0));

    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP + V2(state->backgroundBuffer.width, state->backgroundBuffer.height), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP - V2(state->backgroundBuffer.width, state->backgroundBuffer.height), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP + V2(state->backgroundBuffer.width, -state->backgroundBuffer.height), V2(0, 0));
    DrawBMP(&finalBuffer, &state->backgroundBuffer, BackgroundP + V2(-state->backgroundBuffer.width, state->backgroundBuffer.height), V2(0, 0));
#endif

    BeginCycleCounter(RenderRenderGroup);
    RenderRenderGroup(&renderGroup, &state->finalBuffer);
    EndCycleCounter(RenderRenderGroup);

#if 0
    // NOTE : Test code for normal maps
    DrawBMPSimple(&state->finalBuffer, &state->envMaps[2].LOD,
            V2(50, 800), V2(state->envMaps[2].LOD.width, state->envMaps[2].LOD.height));
    DrawBMPSimple(&state->finalBuffer, &state->envMaps[1].LOD,
            V2(50, 500), V2(state->envMaps[1].LOD.width, state->envMaps[1].LOD.height));
    DrawBMPSimple(&state->finalBuffer, &state->envMaps[0].LOD,
            V2(50, 200), V2(state->envMaps[0].LOD.width, state->envMaps[0].LOD.height));
#endif

    DrawOffScreenBuffer(offscreenBuffer, &state->finalBuffer);

    state->theta += 0.05f;

    EndSimRegion(world, &state->worldArena, &simRegion);

    // NOTE : Temporary memories should all be cleared.
    EndTemporaryMemory(renderGroup.renderMemory);
    CheckMemoryArenaTemporaryMemory(&state->renderArena);
    CheckMemoryArenaTemporaryMemory(&state->worldArena);

    EndCycleCounter(GameUpdateAndRender);
}
extern "C"
GAME_FILL_AUDIO_BUFFER(GameFillAudioBuffer)
{
    u32 ToneHz = 256;
    u32 samplesToFillCountPad = 0;
    // TODO : For now, the game is filling each frame worth of audio. But if the frame takes too much time,
    // there will be a cracking sound because we did not fill the enough audio. What should we do here?
    u32 samplesToFillCount = audioBuffer->channelCount * ((dtPerFrame * (r32)audioBuffer->samplesPerSecond) + samplesToFillCountPad);

    u32 samplesPerOneCycle = (audioBuffer->samplesPerSecond/ToneHz); // Per Cycle == Per One Wave Form

    u32 Volume = 1000;

    for(u32 SampleIndex = 0;
        SampleIndex < samplesToFillCount;
        SampleIndex += audioBuffer->channelCount)
    {
        r32 t = (2.0f*Pi32*audioBuffer->runningSampleIndex)/samplesPerOneCycle;

        r32 SineValue = sinf(t);

        i16 leftSampleValue = (i16)(Volume * SineValue);
        i16 rightSampleValue = leftSampleValue;

        audioBuffer->samples[(audioBuffer->runningSampleIndex++)%audioBuffer->sampleCount] = leftSampleValue;
        audioBuffer->samples[(audioBuffer->runningSampleIndex++)%audioBuffer->sampleCount] = rightSampleValue;
    }

    audioBuffer->isSoundReady = true;
}
