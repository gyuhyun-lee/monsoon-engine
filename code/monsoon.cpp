#include "monsoon_platform_independent.h"
#include "monsoon_intrinsic.h"
#include "monsoon_random.h"
#include "monsoon_world.h"

#include "monsoon_world.cpp"
#include "monsoon_sim_region.cpp"
#include "monsoon_render.cpp"

#include "monsoon.h"
#include <stdio.h>

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
DEBUGLoadBMP(debug_read_entire_file *ReadEntireFile, char *FileName)
{
    // TODO : Currently only supports bmp with compression = 3
    pixel_buffer_32 Result = {};
    debug_platform_read_file_result File = ReadEntireFile(FileName);
    debug_bmp_file_header *Header = (debug_bmp_file_header *)File.memory;
    Assert(Header->Compression == 3);
    u32 RedShift = FindLeastSignificantSetBit(Header->RedMask);
    u32 GreenShift = FindLeastSignificantSetBit(Header->GreenMask);
    u32 BlueShift = FindLeastSignificantSetBit(Header->BlueMask);
    u32 AlphaShift = FindLeastSignificantSetBit(Header->AlphaMask);

    Result.width = Header->width;
    Result.height = Header->height;
    Result.bytesPerPixel = Header->BitsPerPixel/8;
    Result.pitch = Result.width * Result.bytesPerPixel;

    Result.memory = (u32 *)((u8 *)Header + Header->pixelOffset);

    u8 *Sourcerow = (u8 *)Result.memory;
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

        Sourcerow += Result.pitch;
    }
    
    return Result;
}
internal low_entity *
AddLowentity(game_state *state, entity_type Type, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, 
            v3 Dim)
{
    Assert(state->entityCount < ArrayCount(state->entities));

    low_entity *entity = state->entities + state->entityCount++;


    entity->worldP.p = state->world.tileSideInMeters*V2(AbsTileX, AbsTileY) + 
                        0.5f*V2(state->world.tileSideInMeters, state->world.tileSideInMeters) - 
                        0.5f*state->world.chunkDim;
    //entity->worldP.P.Z = 0; // TODO : Properly handle Z
    CanonicalizeWorldPos(&state->world, &entity->worldP);

    entity->dim = Dim;
    entity->type = Type;

    world_chunk *worldChunk = GetWorldChunk(&state->world, entity->worldP.chunkX, entity->worldP.chunkY, entity->worldP.chunkZ);
    PutEntityInsideWorldChunk(worldChunk, &state->worldArena, entity);

    return entity;
}

internal void
AddPlayerentity(game_state *state, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, v3 Dim)
{
    state->player = AddLowentity(state, EntityType_Player, AbsTileX, AbsTileY, AbsTileZ, Dim);
}

internal void
MakeBackground(game_state *state, pixel_buffer_32 *buffer)
{
    ClearPixelBuffer(buffer, 0, 0, 0);

    u32 GroundCount = 500;
    u32 RockCount = 100;
    u32 grassCount = 100;

    random_series Series = Seed(13);
    v2 bufferHalfDim = V2(buffer->width/2, buffer->height/2);

    for(u32 GroundIndex = 0;
        GroundIndex < GroundCount;
        ++GroundIndex)
    {
        u32 RandomNumber = GetNextRandomNumberInSeries(&Series);
        pixel_buffer_32 *BMPToUse = state->groundBMP + RandomNumber%ArrayCount(state->groundBMP);
        v2 pixelP = V2(GetRandomBetween(&Series, 0, buffer->width), GetRandomBetween(&Series, 0, buffer->height));
        v2 BMPBottomLeftCorner = pixelP - BMPToUse->alignment;
        v2 BMPUpperRightCorner = pixelP - BMPToUse->alignment + V2(BMPToUse->width, BMPToUse->height);

        if(!(BMPUpperRightCorner.x > buffer->width ||
            BMPBottomLeftCorner.x < 0 ||
            BMPUpperRightCorner.y > buffer->height  ||
            BMPBottomLeftCorner.y < 0))    
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);
        }
        else
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);

            if(BMPBottomLeftCorner.y < 0 &&
                BMPBottomLeftCorner.x < 0)
            {
                int a = 1;
            }

            // NOTE : Draw the clipped region of the bmp by 
            // simply repositioning the BMPs
            if(BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPUpperRightCorner.x > buffer->width)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.x < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }

            if(BMPBottomLeftCorner.x < 0 &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPBottomLeftCorner.x < 0 &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
        }
    }

#if 1
    for(u32 RockIndex = 0;
        RockIndex < RockCount;
        ++RockIndex)
    {
        u32 RandomNumber = GetNextRandomNumberInSeries(&Series);
        pixel_buffer_32 *BMPToUse = state->rockBMP + RandomNumber%ArrayCount(state->rockBMP);
        v2 pixelP = V2(GetRandomBetween(&Series, 0, buffer->width), GetRandomBetween(&Series, 0, buffer->height));
        v2 BMPBottomLeftCorner = pixelP - BMPToUse->alignment;
        v2 BMPUpperRightCorner = pixelP - BMPToUse->alignment + V2(BMPToUse->width, BMPToUse->height);

        if(!(BMPUpperRightCorner.x > buffer->width ||
            BMPBottomLeftCorner.x < 0 ||
            BMPUpperRightCorner.y > buffer->height  ||
            BMPBottomLeftCorner.y < 0))    
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);
        }
        else
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);

            if(BMPBottomLeftCorner.y < 0 &&
                BMPBottomLeftCorner.x < 0)
            {
                int a = 1;
            }

            // NOTE : Draw the clipped region of the bmp by 
            // simply repositioning the BMPs
            if(BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPUpperRightCorner.x > buffer->width)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.x < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }

            if(BMPBottomLeftCorner.x < 0 &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPBottomLeftCorner.x < 0 &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
        }
    }

    for(u32 grassIndex = 0;
        grassIndex < grassCount;
        ++grassIndex)
    {
        u32 RandomNumber = GetNextRandomNumberInSeries(&Series);
        pixel_buffer_32 *BMPToUse = state->grassBMP + RandomNumber%ArrayCount(state->grassBMP);
        v2 pixelP = V2(GetRandomBetween(&Series, 0, buffer->width), GetRandomBetween(&Series, 0, buffer->height));
        v2 BMPBottomLeftCorner = pixelP - BMPToUse->alignment;
        v2 BMPUpperRightCorner = pixelP - BMPToUse->alignment + V2(BMPToUse->width, BMPToUse->height);

        if(!(BMPUpperRightCorner.x > buffer->width ||
            BMPBottomLeftCorner.x < 0 ||
            BMPUpperRightCorner.y > buffer->height  ||
            BMPBottomLeftCorner.y < 0))    
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);
        }
        else
        {
            DrawBMP(buffer, BMPToUse, pixelP, V2(0, 0), BMPToUse->alignment);

            if(BMPBottomLeftCorner.y < 0 &&
                BMPBottomLeftCorner.x < 0)
            {
                int a = 1;
            }

            // NOTE : Draw the clipped region of the bmp by 
            // simply repositioning the BMPs
            if(BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPUpperRightCorner.x > buffer->width)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            if(BMPBottomLeftCorner.x < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }

            if(BMPBottomLeftCorner.x < 0 &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPBottomLeftCorner.x < 0 &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x += buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPUpperRightCorner.y > buffer->height)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y -= buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
            else if(BMPUpperRightCorner.x > buffer->width &&
                BMPBottomLeftCorner.y < 0)
            {
                v2 newPixelP = pixelP;
                newPixelP.x -= buffer->width;
                newPixelP.y += buffer->height;
                DrawBMP(buffer, BMPToUse, newPixelP, V2(0, 0), BMPToUse->alignment);
            }
        }
    }
#endif
}

// TODO : This should go away!
#define TILE_COUNT_X 17
#define TILE_COUNT_Y 9
extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->permanentStorageSize);

    game_state *state = (game_state *)Memory->permanentStorage;

    if(!state->isInitialized)
    {
        state->worldArena = StartMemoryArena((u8 *)Memory->transientStorage, Megabytes(256));

        world *world = &state->world;
        world->tileSideInMeters = 2.0f;
        world->chunkDim.x = TILE_COUNT_X*world->tileSideInMeters;
        world->chunkDim.y = TILE_COUNT_Y*world->tileSideInMeters;
        InitializeWorld(world);

        state->cameraPos.chunkX = 0;
        state->cameraPos.chunkY = 0;
        state->cameraPos.chunkZ = 0;
        state->cameraPos.p.x = 0;
        state->cameraPos.p.y = 0;

        AddPlayerentity(state, 6, 4, 0, 
                        V3(0.5f*world->tileSideInMeters, 0.9*world->tileSideInMeters, 1));

        random_series Series = Seed(123);
        // TODO : Proper map construction!
        // NOTE : Construction starts with bottom map
        b32 LeftShouldBeOpened = false;
        b32 BottomShouldBeOpened = false;
        b32 RightShouldBeOpened = false;
        b32 UpShouldBeOpened = true;

        u32 LeftBottomTileX = 0;
        u32 LeftBottomTileY = 0;
        for(u32 ScreenIndex = 0;
            ScreenIndex < 100;
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
                    b32 ShouldAddWall = true;

                    if(LeftShouldBeOpened)
                    {
                        if(Y == TILE_COUNT_Y/2 && X == 0)
                        {
                            ShouldAddWall = false;
                        }
                    }
                    if(RightShouldBeOpened)
                    {
                        if(Y == TILE_COUNT_Y/2 && X == TILE_COUNT_X-1)
                        {
                            ShouldAddWall = false;
                        }
                    }
                    if(BottomShouldBeOpened)
                    {
                        if(X == TILE_COUNT_X/2 && Y == 0)
                        {
                            ShouldAddWall = false;
                        }
                    }
                    if(UpShouldBeOpened)
                    {
                        if(X == TILE_COUNT_X/2 && Y == TILE_COUNT_Y-1)
                        {
                            ShouldAddWall = false;
                        }
                    }

                    if(!(X == 0 || X == TILE_COUNT_X - 1) && !(Y == 0 || Y == TILE_COUNT_Y - 1))
                    {
                        ShouldAddWall = false;
                    }

                    if(ShouldAddWall)
                    {
                        AddLowentity(state, EntityType_Wall, column, row, 0, 
                                    V3(world->tileSideInMeters, world->tileSideInMeters, 1));
                        printf("%u\n", state->entityCount);
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

        state->headBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_head.bmp");
        state->headBMP.alignment = V2(48, 40);
        state->torsoBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_torso.bmp");
        state->torsoBMP.alignment = V2(48, 40);
        state->capeBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_cape.bmp");
        state->capeBMP.alignment = V2(48, 40);

        state->rockBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock00.bmp");
        state->rockBMP[0].alignment = V2(35, 40);
        state->rockBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock01.bmp");
        state->rockBMP[1].alignment = V2(35, 40);
        state->rockBMP[2] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock02.bmp");
        state->rockBMP[2].alignment = V2(35, 40);
        state->rockBMP[3] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock03.bmp");
        state->rockBMP[3].alignment = V2(35, 40);

        state->groundBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground00.bmp");
        state->groundBMP[0].alignment = V2(136, 60);
        state->groundBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground01.bmp");
        state->groundBMP[1].alignment = V2(136, 60);
        state->groundBMP[2] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground02.bmp");
        state->groundBMP[2].alignment = V2(136, 60);
        state->groundBMP[3] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/Ground03.bmp");
        state->groundBMP[3].alignment = V2(136, 60);

        state->grassBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass00.bmp");
        state->grassBMP[0].alignment = V2(82, 65);
        state->grassBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass01.bmp");
        state->grassBMP[1].alignment = V2(82, 65);

        state->renderArena = StartMemoryArena((u8 *)Memory->transientStorage + Megabytes(256), Megabytes(256));
        state->backgroundBuffer.width = 800;
        state->backgroundBuffer.height = 600;
        state->backgroundBuffer.bytesPerPixel = 4;
        state->backgroundBuffer.pitch = state->backgroundBuffer.width*state->backgroundBuffer.bytesPerPixel;
        state->backgroundBuffer.memory = (u32 *)PushSize(&state->renderArena, state->backgroundBuffer.pitch*state->backgroundBuffer.height);
        //MakeBackground(state, &state->backgroundBuffer);

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
    }

    v2 ddPlayer = {};
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
    sim_region simRegion = {};
    // TODO : Accurate max entity delta for the sim region!
    StartSimRegion(world, &simRegion, state->cameraPos, world->chunkDim, V2(10, 10));

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

    render_group renderGroup = {};
    // TODO : How can I keep track of used memory without explicitly mentioning it?
    renderGroup.renderMemory = StartTemporaryMemory(&state->renderArena, Megabytes(16));
    renderGroup.metersToPixels = (r32)Buffer->width/((TILE_COUNT_X+1)*world->tileSideInMeters);
    //renderGroup.MetersToPixels = 10;
    renderGroup.bufferHalfDim = 0.5f*V2(Buffer->width, Buffer->height);

    for(u32 entityIndex = 0;
        entityIndex < simRegion.entityCount;
        ++entityIndex)
    {
        sim_entity *entity = simRegion.entities + entityIndex;

        switch(entity->type)
        {
            case EntityType_Wall:
            {
                PushRect(&renderGroup, entity->p, entity->dim, V3(1.0f, 1.0f, 1.0f));
            }break;

            case EntityType_Player: 
            {
                v2 xAxis = V2(Cos(state->theta), Sin(state->theta));
                v2 yAxis = V2(-Sin(state->theta), Cos(state->theta));
                PushRect(&renderGroup, entity->p, entity->dim, V3(0.0f, 0.8f, 1.0f));
                PushBMP(&renderGroup, &state->headBMP, entity->p, 1*entity->dim, xAxis, yAxis);
                PushBMP(&renderGroup, &state->torsoBMP, entity->p, 1*entity->dim, xAxis, yAxis);
                PushBMP(&renderGroup, &state->capeBMP, entity->p, 1*entity->dim, xAxis, yAxis);
            }break;
        }
    }

    EndSimRegion(world, &state->worldArena, &simRegion);
    pixel_buffer_32 finalBuffer = MakeEmptyPixelBuffer32(Buffer->width, Buffer->height);
    temporary_memory BufferTemporaryMemory = StartTemporaryMemory(&state->renderArena, finalBuffer.height*finalBuffer.pitch);
    finalBuffer.memory = (u32 *)BufferTemporaryMemory.base;
    ClearPixelBuffer(&finalBuffer, 0.6, 0.6, 0.6);

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

    RenderRenderGroup(&renderGroup, &finalBuffer);

    DrawOffScreenBuffer(Buffer, &finalBuffer);

    state->theta += 0.01f;

    // NOTE : Temporary memories should all be cleared.
    EndTemporaryMemory(BufferTemporaryMemory);
    EndTemporaryMemory(renderGroup.renderMemory);
    CheckMemoryArenaTemporaryMemory(&state->renderArena);
    CheckMemoryArenaTemporaryMemory(&state->worldArena);
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
