#include "monsoon_platform_independent.h"
#include "monsoon_intrinsic.h"
#include "monsoon_random.h"
#include "monsoon_world.h"

#include "monsoon_world.cpp"
#include "monsoon_sim_region.cpp"
#include "monsoon_render.cpp"

#include "monsoon.h"
#include <stdio.h>

internal void 
RenderSomething(game_offscreen_buffer *Buffer, i32 XOffset = 0, i32 YOffset = 0)
{
    u8 *FirstPixelOfRow = (u8 *)Buffer->Memory;
    for(int Row = 0;
        Row < Buffer->Height;
        ++Row)
    {
        u32 *Pixel = (u32 *)(FirstPixelOfRow);
        for(int Column = 0;
            Column < Buffer->Width;
            ++Column)
        {
            *Pixel++ = ((XOffset + Column) << 8)|((YOffset + Row) << 16);
        }

        FirstPixelOfRow += Buffer->Pitch;
    }
}

#pragma pack(push, 1)
struct debug_bmp_file_header
{
    u16 FileHeader;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 PixelOffset;
    u32 HeaderSize;
    u32 Width;
    u32 Height;
    u16 ColorPlaneCount;
    u16 BitsPerPixel;
    u32 Compression;
};
#pragma pack(pop)

internal debug_loaded_bmp
DEBUGLoadBMP(debug_read_entire_file *ReadEntireFile, char *FileName)
{
    // TODO : Might have to get the shift values of all the color channels 
    // based on the compression method
    debug_loaded_bmp Result = {};
    debug_platform_read_file_result File = ReadEntireFile(FileName);
    debug_bmp_file_header *Header = (debug_bmp_file_header *)File.Memory;

    Result.Width = Header->Width;
    Result.Height = Header->Height;
    Result.BytesPerPixel = Header->BitsPerPixel/8;
    Result.Pitch = Result.Width * Result.BytesPerPixel;

    Result.Pixels = (u32 *)((u8 *)Header + Header->PixelOffset);
    
    return Result;
}
internal low_entity *
AddLowEntity(game_state *State, entity_type Type, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, 
            v2 Dim)
{
    Assert(State->EntityCount < ArrayCount(State->Entities));

    low_entity *Entity = State->Entities + State->EntityCount++;

    Entity->WorldP.P = State->World.TileSideInMeters*V2(AbsTileX, AbsTileY) + 
                        0.5f*V2(State->World.TileSideInMeters, State->World.TileSideInMeters) - 
                        0.5f*State->World.ChunkDim;
    //Entity->WorldP.P.Z = 0; // TODO : Properly handle Z
    CanonicalizeWorldPos(&State->World, &Entity->WorldP);

    Entity->Dim = Dim;
    Entity->Type = Type;

    world_chunk *WorldChunk = GetWorldChunk(&State->World, Entity->WorldP.ChunkX, Entity->WorldP.ChunkY, Entity->WorldP.ChunkZ);
    PutEntityInsideWorldChunk(WorldChunk, &State->WorldArena, Entity);

    return Entity;
}

internal void
AddPlayerEntity(game_state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, v2 Dim)
{
    State->Player = AddLowEntity(State, EntityType_Player, AbsTileX, AbsTileY, AbsTileZ, Dim);
}

internal void
TestWall(r32 WallX, r32 WallHalfDimY, v2 WallNormal, 
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
                if(tY > -WallHalfDimY && tY < WallHalfDimY)
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
MoveEntity(game_state *State, sim_region *SimRegion, 
            sim_entity *Entity, v2 ddP, r32 Speed, r32 dtPerFrame)
{
    r32 ddPLength = LengthSquare(ddP);
    if(ddPLength > 1.0f)
    {
        ddP *= 1.0f/SquareRoot2(ddPLength);
    }

    ddP *= Speed;
    ddP -= 8.0f*Entity->dP;
    /*
     * NOTE :
     * Position = 0.5f*a*dt*dt + previous frame v * dt + previous frame p
     * Velocity = a*dt + v;
    */   
    v2 EntityDelta = 0.5f*Square(dtPerFrame)*ddP + 
                    dtPerFrame*Entity->dP;
    v2 RemainingEntityDelta = EntityDelta;
    Entity->dP = dtPerFrame*ddP + Entity->dP;
    r32 DistanceLeftSquare = LengthSquare(EntityDelta);

    for(u32 CollisionIteration = 0;
        CollisionIteration < 4;
        ++CollisionIteration)
    {
        if(DistanceLeftSquare > 0.0f)
        {
            v2 NewEntityPos = Entity->P;
            NewEntityPos += RemainingEntityDelta;

            r32 tMin = 1.0f;
            v2 HitWallNormal = {};
            b32 Hit = false;

            for(u32 EntityIndex = 0;
                EntityIndex < SimRegion->EntityCount;
                ++EntityIndex)
            {
                sim_entity *TestEntity = SimRegion->Entities + EntityIndex;
                if(TestEntity != Entity)
                {
                    v2 MinkowskiHalfDim = 0.5f*(TestEntity->Dim + Entity->Dim);
                    v2 TestEntityRelNewP = NewEntityPos - TestEntity->P;
                    v2 TestEntityRelOldP = Entity->P - TestEntity->P;

                    // NOTE : Test against left wall
                    TestWall(-MinkowskiHalfDim.X, MinkowskiHalfDim.Y, V2(-1, 0), 
                            TestEntityRelNewP.X, TestEntityRelOldP.X, TestEntityRelNewP.Y, TestEntityRelOldP.Y, 
                            &tMin, &Hit, &HitWallNormal);

                    // NOTE : Test against right wall
                    TestWall(MinkowskiHalfDim.X, MinkowskiHalfDim.Y, V2(1, 0), 
                            TestEntityRelNewP.X, TestEntityRelOldP.X, TestEntityRelNewP.Y, TestEntityRelOldP.Y, 
                            &tMin, &Hit, &HitWallNormal);

                    // NOTE : Test against upper wall
                    TestWall(MinkowskiHalfDim.Y, MinkowskiHalfDim.X, V2(0, -1), 
                            TestEntityRelNewP.Y, TestEntityRelOldP.Y, TestEntityRelNewP.X, TestEntityRelOldP.X, 
                            &tMin, &Hit, &HitWallNormal);

                    // NOTE : Test against bottom wall
                    TestWall(-MinkowskiHalfDim.Y, MinkowskiHalfDim.X, V2(0, 1), 
                            TestEntityRelNewP.Y, TestEntityRelOldP.Y, TestEntityRelNewP.X, TestEntityRelOldP.X, 
                            &tMin, &Hit, &HitWallNormal);
                }
            }

            v2 EntityDeltaForThisIteration = RemainingEntityDelta;
            v2 EntityDeltaLeftForThisIteration = V2(0, 0);
            v2 OldRemainingEntityDelta = RemainingEntityDelta;

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

                v2 EntityDeltaLeftForThisIteration = RemainingEntityDelta - EntityDeltaForThisIteration;

                Entity->dP = Entity->dP - 1.0f*Dot(Entity->dP, HitWallNormal)*HitWallNormal;

                RemainingEntityDelta = EntityDeltaLeftForThisIteration - 1.0f*Dot(EntityDeltaLeftForThisIteration, HitWallNormal)*HitWallNormal;
            }

            Entity->P += EntityDeltaForThisIteration;

            // TODO : Clean this code!
            r32 LengthSquareMovedForThisIteration = LengthSquare(EntityDeltaForThisIteration);
            r32 LengthSquareCanceledByWall = LengthSquare(EntityDeltaLeftForThisIteration - RemainingEntityDelta);
            DistanceLeftSquare -= LengthSquareMovedForThisIteration + LengthSquareCanceledByWall;
        }
    }

    //CanonicalizeWorldPos(World, &Entity->WorldP, EntityDelta);
}

internal void
DrawBackground(game_state *State, game_offscreen_buffer *Buffer)
{
    random_series Series = Seed(13);

    GetNextRandomNumberInSeries(&Series);
}

// TODO : This should go away!
#define TILE_COUNT_X 17
#define TILE_COUNT_Y 9
extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorage;

    if(!State->IsInitialized)
    {
        State->WorldArena = StartMemoryArena((u8 *)Memory->TransientStorage, Megabytes(16));

        world *World = &State->World;
        World->TileSideInMeters = 2.0f;
        World->ChunkDim.X = TILE_COUNT_X*World->TileSideInMeters;
        World->ChunkDim.Y = TILE_COUNT_Y*World->TileSideInMeters;
        InitializeWorld(World);

        State->XOffset = 0;
        State->YOffset = 0;

        State->CameraPos.ChunkX = 0;
        State->CameraPos.ChunkY = 0;
        State->CameraPos.ChunkZ = 0;
        State->CameraPos.P.X = 0;
        State->CameraPos.P.Y = 0;

        AddPlayerEntity(State, 6, 4, 0, V2(0.5f*World->TileSideInMeters, 0.9*World->TileSideInMeters));

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
                u32 Row = LeftBottomTileY + Y;
                for(u32 X = 0;
                    X < TILE_COUNT_X;
                    ++X)
                {
                    u32 Column = LeftBottomTileX + X;
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
                        AddLowEntity(State, EntityType_Wall, Column, Row, 0, V2(World->TileSideInMeters, World->TileSideInMeters));
                        printf("%u\n", State->EntityCount);
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

        State->HeadBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_head.bmp");
        State->HeadBMP.Alignment = V2(48, 40);
        State->TorsoBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_torso.bmp");
        State->TorsoBMP.Alignment = V2(48, 40);
        State->CapeBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test/test_hero_front_cape.bmp");
        State->CapeBMP.Alignment = V2(48, 40);

        State->RockBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock00.bmp");
        State->RockBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock01.bmp");
        State->RockBMP[2] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock02.bmp");
        State->RockBMP[3] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/rock03.bmp");

        State->GrassBMP[0] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass00.bmp");
        State->GrassBMP[1] = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test2/grass01.bmp");

        State->IsInitialized = true;
    }
    world *World = &State->World;

    game_input Input = {}; // NOTE : Sum of all the raw inputs
    for(u32 ControllerIndex = 0;
        ControllerIndex < RawInput->ControllerCount;
        ++ControllerIndex)
    {
        game_controller *RawController = RawInput->Controllers + ControllerIndex;
        if(RawController->IsAnalog)
        {
            // NOTE : This is a controller
            if(RawController->LeftStickX > 0.3f || RawController->DPadRight)
            {
                Input.MoveRight = true;
            }
            if(RawController->LeftStickX < -0.3f || RawController->DPadLeft)
            {
                Input.MoveLeft = true;
            }
            if(RawController->LeftStickY > 0.3f || RawController->DPadUp)
            {
                Input.MoveUp = true;
            }
            if(RawController->LeftStickY < -0.3f || RawController->DPadDown)
            {
                Input.MoveDown = true;
            }
        }
        else
        {
            // NOTE : This is keyboard
            if(RawController->MoveUp)
            {
                Input.MoveUp = RawController->MoveUp||Input.MoveUp;
            }
            if(RawController->MoveDown)
            {
                Input.MoveDown = RawController->MoveDown||Input.MoveDown;
            }
            if(RawController->MoveLeft)
            {
                Input.MoveLeft = RawController->MoveLeft||Input.MoveLeft;
            }
            if(RawController->MoveRight)
            {
                Input.MoveRight = RawController->MoveRight||Input.MoveRight;
            }
        }

        if(RawController->AButton)
        {
            Input.ActionRight = RawController->AButton||Input.ActionRight;
        }
    }

    v2 ddPlayer = {};
    if(Input.MoveRight)
    {
        ddPlayer.X += 1.0f;
    }
    if(Input.MoveLeft)
    {
        ddPlayer.X -= 1.0f;
    }
    if(Input.MoveUp)
    {
        ddPlayer.Y += 1.0f;
    }
    if(Input.MoveDown)
    {
        ddPlayer.Y -= 1.0f;
    }

    // TODO : This is wrong when the player is using controller.
    if(ddPlayer.X != 0.0f && ddPlayer.Y != 0.0f)
    {
        ddPlayer *= 0.70710678118f;
    }

    r32 PlayerSpeed = 50.0f;
    if(Input.ActionRight)
    {
        PlayerSpeed = 150.f;
    }

    State->CameraPos = State->Player->WorldP;
    sim_region SimRegion = {};
    // TODO : Accurate max entity delta for the sim region!
    StartSimRegion(World, &SimRegion, State->CameraPos, World->ChunkDim, V2(10, 10));

    for(u32 EntityIndex = 0;
        EntityIndex < SimRegion.EntityCount;
        ++EntityIndex)
    {
        sim_entity *Entity = SimRegion.Entities + EntityIndex;

        switch(Entity->Type)
        {
            case EntityType_Player:
            {
                MoveEntity(State, &SimRegion, Entity, ddPlayer, PlayerSpeed, dtPerFrame);
            }break;
        }
    }

    render_group RenderGroup = {};
    // TODO : How can I keep track of used memory without explicitly mentioning it?
    RenderGroup.Arena = StartMemoryArena((u8 *)Memory->TransientStorage + Megabytes(16), Megabytes(4));
    RenderGroup.MetersToPixels = (r32)Buffer->Width/((TILE_COUNT_X+1)*World->TileSideInMeters);
    //RenderGroup.MetersToPixels = 10;
    RenderGroup.BufferHalfDim = 0.5f*V2(Buffer->Width, Buffer->Height);

    for(u32 EntityIndex = 0;
        EntityIndex < SimRegion.EntityCount;
        ++EntityIndex)
    {
        sim_entity *Entity = SimRegion.Entities + EntityIndex;

        switch(Entity->Type)
        {
            case EntityType_Wall:
            {
                PushRect(&RenderGroup, Entity->P, Entity->Dim, V3(1.0f, 1.0f, 1.0f));
            }break;

            case EntityType_Player: 
            {
#if 1
                PushRect(&RenderGroup, Entity->P, Entity->Dim, V3(0.0f, 0.8f, 1.0f));
#else
                PushBMP(&RenderGroup, &State->HeadBMP, Entity->P, Entity->Dim);
                PushBMP(&RenderGroup, &State->TorsoBMP, Entity->P, Entity->Dim);
                PushBMP(&RenderGroup, &State->CapeBMP, Entity->P, Entity->Dim);
#endif
            }break;
        }
    }

    EndSimRegion(World, &State->WorldArena, &SimRegion);

    RenderRenderGroup(&RenderGroup, Buffer);
}

extern "C"
GAME_FILL_AUDIO_BUFFER(GameFillAudioBuffer)
{
    u32 ToneHz = 256;
    u32 SamplesToFillCountPad = 0;
    // TODO : For now, the game is filling each frame worth of audio. But if the frame takes too much time,
    // there will be a cracking sound because we did not fill the enough audio. What should we do here?
    u32 SamplesToFillCount = AudioBuffer->ChannelCount * ((dtPerFrame * (r32)AudioBuffer->SamplesPerSecond) + SamplesToFillCountPad);

    u32 SamplesPerOneCycle = (AudioBuffer->SamplesPerSecond/ToneHz); // Per Cycle == Per One Wave Form

    u32 Volume = 1000;

    for(u32 SampleIndex = 0;
        SampleIndex < SamplesToFillCount;
        SampleIndex += AudioBuffer->ChannelCount)
    {
        r32 t = (2.0f*Pi32*AudioBuffer->RunningSampleIndex)/SamplesPerOneCycle;

        r32 SineValue = sinf(t);

        i16 LeftSampleValue = (i16)(Volume * SineValue);
        i16 RightSampleValue = LeftSampleValue;

        AudioBuffer->Samples[(AudioBuffer->RunningSampleIndex++)%AudioBuffer->SampleCount] = LeftSampleValue;
        AudioBuffer->Samples[(AudioBuffer->RunningSampleIndex++)%AudioBuffer->SampleCount] = RightSampleValue;
    }

    AudioBuffer->IsSoundReady = true;
}
