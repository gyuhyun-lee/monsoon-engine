#include "monsoon_platform_independent.h"
#include "monsoon_intrinsic.h"
#include "monsoon_tile.cpp"

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

// TODO : Better buffer clearing function
internal void 
ClearBuffer(game_offscreen_buffer *Buffer)
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
            *Pixel++ = 0x00000000;
        }

        FirstPixelOfRow += Buffer->Pitch;
    }
}


// NOTE : MacOS offscreen buffer is bottom-up 
internal void
DrawRectangle(game_offscreen_buffer *Buffer, r32 X, r32 Y, r32 Width, r32 Height,
            r32 R, r32 G, r32 B)
{    
    i32 MinX = RoundR32ToInt32(X);
    i32 MinY = RoundR32ToInt32(Y);
    i32 MaxX = RoundR32ToInt32(X + Width);
    i32 MaxY = RoundR32ToInt32(Y + Height);

    i32 DEBUGMinX = RoundR32ToInt32(X);
    i32 DEBUGMinY = RoundR32ToInt32(Y);
    i32 DEBUGMaxX = RoundR32ToInt32(X + Width);
    i32 DEBUGMaxY = RoundR32ToInt32(Y + Height);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
 
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }   

    // NOTE : Bit pattern for the pixel : AARRGGBB
    u32 Color = (u32)((RoundR32ToInt32(R * 255.0f) << 16) |
                (RoundR32ToInt32(G * 255.0f) << 8) |
                (RoundR32ToInt32(B * 255.0f) << 0));

    u8 *Row = (u8 *)Buffer->Memory + 
                    Buffer->Pitch*MinY + 
                    Buffer->BytesPerPixel*MinX;
    for(i32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i32 X = MinX;
            X < MaxX;
            ++X)
        {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
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

internal void
DrawBMP(game_offscreen_buffer *Buffer, debug_loaded_bmp *LoadedBMP, 
        r32 X, r32 Y, r32 Width = 0, r32 Height = 0,
        r32 AlignmentX = 0, r32 AlignmentY = 0)
{
    X -= AlignmentX;
    Y -= AlignmentY;
    i32 MinX = RoundR32ToInt32(X);
    i32 MinY = RoundR32ToInt32(Y);
    i32 MaxX = RoundR32ToInt32(X + LoadedBMP->Width);
    i32 MaxY = RoundR32ToInt32(Y + LoadedBMP->Height);

    i32 SourceOffsetX = 0;
    i32 SourceOffsetY = 0;
    
    if(MinX < 0) 
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }   

#if 0
    // NOTE : To see the region of the whole BMP, enable this.
    DrawRectangle(Buffer, X, Y, LoadedBMP->Width, LoadedBMP->Height, 1, 0, 0);
#endif

    u8 *Row = (u8 *)Buffer->Memory + 
                    Buffer->Pitch*MinY + 
                    Buffer->BytesPerPixel*MinX;
    u8 *SourceRow = (u8 *)LoadedBMP->Pixels + 
                    LoadedBMP->Pitch*SourceOffsetY + 
                    LoadedBMP->BytesPerPixel*SourceOffsetX;

    for(i32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        u32 *SourcePixel = (u32 *)SourceRow;
        for(i32 X = MinX;
            X < MaxX;
            ++X)
        {
            // NOTE : Bit pattern for the pixel : AARRGGBB
            r32 DestR = ((*Pixel >> 16) & 0x000000ff) / 255.0f;
            r32 DestG = ((*Pixel >> 8) & 0x000000ff) / 255.0f;
            r32 DestB = ((*Pixel >> 0) & 0x000000ff) / 255.0f;

            // NOTE : Bit pattern for the BMP : AARRGGBB
            r32 SourceR = ((*SourcePixel >> 16) & 0x000000ff) / 255.0f;
            r32 SourceG = ((*SourcePixel >> 8) & 0x000000ff) / 255.0f;
            r32 SourceB = ((*SourcePixel >> 0) & 0x000000ff) / 255.0f;
            r32 SourceA = ((*SourcePixel >> 24) & 0x000000ff) / 255.0f;

            r32 ResultR = (1.0f-SourceA)*DestR + SourceA*SourceR;
            r32 ResultG = (1.0f-SourceA)*DestG + SourceA*SourceG;
            r32 ResultB = (1.0f-SourceA)*DestB + SourceA*SourceB;

            u32 ResultColor = ((u32)(ResultR*255.0f + 0.5f) << 16) |
                                ((u32)(ResultG*255.0f + 0.5f) << 8) |
                                ((u32)(ResultB*255.0f + 0.5f) << 0);

            *Pixel++ = ResultColor;
            SourcePixel++;
        }



        Row += Buffer->Pitch;
        SourceRow += LoadedBMP->Pitch;
    }
}



internal low_entity *
AddLowEntity(game_state *State, entity_type Type, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, r32 Width, r32 Height)
{
    Assert(State->EntityCount < ArrayCount(State->Entities));

    low_entity *Entity = State->Entities + State->EntityCount++;

    Entity->WorldP.AbsTileX = AbsTileX;
    Entity->WorldP.AbsTileY = AbsTileY;
    Entity->WorldP.AbsTileZ = AbsTileZ;
    Entity->Width = Width;
    Entity->Height = Height;
    Entity->Type = Type;

    return Entity;
}

internal void
AddPlayerEntity(game_state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, r32 Width, r32 Height)
{
    State->Player = AddLowEntity(State, Entity_Type_Player, AbsTileX, AbsTileY, AbsTileZ, Width, Height);
}

internal void
AddTilesAsLowEntity(game_state  *State, tile_chunk *TileChunk, r32 TileSideInMeters)
{
    for(u32 Y = TileChunk->MinAbsTileY;
        Y < TileChunk->MaxAbsTileY;
        ++Y)
    {
        for(u32 X = TileChunk->MinAbsTileX;
            X < TileChunk->MaxAbsTileX;
            ++X)
        {
            if(GetTileValueFromTileChunkUnchecked(TileChunk, X, Y))
            {
                AddLowEntity(State, Entity_Type_Wall, X, Y, 0, TileSideInMeters, TileSideInMeters);
            }
        }
    }
}

// TODO : This should go away!
#define TILE_COUNT_X 17
#define TILE_COUNT_Y 9
extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) < Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorage;
    u32 Tiles00[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 0, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 1, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 0},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 0},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 Tiles01[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 0,     0, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 1, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {0, 0, 0, 0,     0, 1, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 1,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 Tiles10[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 0},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 0,     0, 0, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 Tiles11[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 0,     0, 0, 1, 1,     1, 1, 1, 1, 1}
    };


    if(!State->IsInitialized)
    {
        // TODO : Tile Chunk Construction
        world *World = &State->World;
        World->TileSideInMeters = 2.0f;
        World->TileChunks[0].Tiles = (u32 *)Tiles00;
        World->TileChunks[0].MinAbsTileX = 0;
        World->TileChunks[0].MinAbsTileY = 0;
        World->TileChunks[0].MaxAbsTileX = World->TileChunks[0].MinAbsTileX + TILE_COUNT_X;
        World->TileChunks[0].MaxAbsTileY = World->TileChunks[0].MinAbsTileY + TILE_COUNT_Y;
        AddTilesAsLowEntity(State, World->TileChunks + 0, World->TileSideInMeters);

        World->TileChunks[1].MinAbsTileX = TILE_COUNT_X;
        World->TileChunks[1].MinAbsTileY = 0;
        World->TileChunks[1].Tiles = (u32 *)Tiles01;
        World->TileChunks[1].MaxAbsTileX = World->TileChunks[1].MinAbsTileX + TILE_COUNT_X;
        World->TileChunks[1].MaxAbsTileY = World->TileChunks[1].MinAbsTileY + TILE_COUNT_Y;
        AddTilesAsLowEntity(State, World->TileChunks + 1, World->TileSideInMeters);

        World->TileChunks[2].MinAbsTileX = 0;
        World->TileChunks[2].MinAbsTileY = TILE_COUNT_Y;
        World->TileChunks[2].Tiles = (u32 *)Tiles10;
        World->TileChunks[2].MaxAbsTileX = World->TileChunks[2].MinAbsTileX + TILE_COUNT_X;
        World->TileChunks[2].MaxAbsTileY = World->TileChunks[2].MinAbsTileY + TILE_COUNT_Y;
        AddTilesAsLowEntity(State, World->TileChunks + 2, World->TileSideInMeters);

        World->TileChunks[3].MinAbsTileX = TILE_COUNT_X;
        World->TileChunks[3].MinAbsTileY = TILE_COUNT_Y;
        World->TileChunks[3].Tiles = (u32 *)Tiles11;
        World->TileChunks[3].MaxAbsTileX = World->TileChunks[3].MinAbsTileX + TILE_COUNT_X;
        World->TileChunks[3].MaxAbsTileY = World->TileChunks[3].MinAbsTileY + TILE_COUNT_Y;
        AddTilesAsLowEntity(State, World->TileChunks + 3, World->TileSideInMeters);

        State->XOffset = 0;
        State->YOffset = 0;

        State->CameraPos.AbsTileX = (TILE_COUNT_X+1)/2;
        State->CameraPos.AbsTileY = (TILE_COUNT_Y+1)/2;
        State->CameraPos.P.X = 0;
        State->CameraPos.P.Y = 0;

        AddPlayerEntity(State, 6, 4, 0, 0.5f*World->TileSideInMeters, 0.9*World->TileSideInMeters);

        State->HeadBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test_hero_front_head.bmp");
        State->HeadBMP.AlignmentX = 48;
        State->HeadBMP.AlignmentY = 40;
        State->TorsoBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test_hero_front_torso.bmp");
        State->TorsoBMP.AlignmentX = 48;
        State->TorsoBMP.AlignmentY = 40;
        State->CapeBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/test_hero_front_cape.bmp");
        State->CapeBMP.AlignmentX = 48;
        State->CapeBMP.AlignmentY = 40;

        State->IsInitialized = true;
    }
    world *World = &State->World;

    r32 MetersToPixels = (r32)Buffer->Width/((TILE_COUNT_X+1)*World->TileSideInMeters);

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

    if(ddPlayer.X != 0.0f && ddPlayer.Y != 0.0f)
    {
        // TODO : This only works when 
        ddPlayer *= 0.70710678118f;
    }

    v2 PlayerDim = V2(State->Player->Width, State->Player->Height);

    r32 PlayerSpeed = 50.0f;
    if(Input.ActionRight)
    {
        PlayerSpeed = 50.f;
    }

    ddPlayer *= PlayerSpeed;
    /*
     * NOTE :
     * Position = 0.5f*a*dt*dt + previous frame v * dt + previous frame p
     * Velocity = a*dt + v;
    */   
#if 1
    // TODO : Something is wrong with the equation?
    //ddPlayer -= 5.0f*State->dPlayer;
    ddPlayer -= 8.0f*State->dPlayer;

    world_position NewPlayerPos = State->Player->WorldP;
    NewPlayerPos.P = 0.5f*Square(dtPerFrame)*ddPlayer + 
                    dtPerFrame*State->dPlayer + 
                    NewPlayerPos.P;
    State->dPlayer = dtPerFrame*ddPlayer + State->dPlayer;

    CanonicalizeWorldPos(World, &NewPlayerPos);
#else
    world_position NewPlayerPos = State->Player->WorldP;
    CanonicalizeWorldPos(World, &NewPlayerPos, dtPerFrame*ddPlayer);
#endif
    
    world_position NewPlayerPosUpperLeft = NewPlayerPos;
    CanonicalizeWorldPos(World, &NewPlayerPosUpperLeft, V2(-0.5f*PlayerDim.X, 0.5f*PlayerDim.Y));
     
    world_position NewPlayerPosUpperRight = NewPlayerPos;
    CanonicalizeWorldPos(World, &NewPlayerPosUpperRight, V2(0.5f*PlayerDim.X, 0.5f*PlayerDim.Y));

    world_position NewPlayerPosBottonRight = NewPlayerPos;
    CanonicalizeWorldPos(World, &NewPlayerPosBottonRight, V2(0.5f*PlayerDim.X, -0.5f*PlayerDim.Y));

    world_position NewPlayerPosBottomLeft = NewPlayerPos;
    CanonicalizeWorldPos(World, &NewPlayerPosBottomLeft, V2(-0.5f*PlayerDim.X, -0.5f*PlayerDim.Y));

    if(IsWorldPointEmptyUnchecked(World, NewPlayerPos) &&
        IsWorldPointEmptyUnchecked(World, NewPlayerPosUpperLeft) &&
        IsWorldPointEmptyUnchecked(World, NewPlayerPosUpperRight) &&
        IsWorldPointEmptyUnchecked(World, NewPlayerPosBottomLeft) &&
        IsWorldPointEmptyUnchecked(World, NewPlayerPosBottonRight))
    {
        State->Player->WorldP = NewPlayerPos;
    }

    ClearBuffer(Buffer);

    r32 ScreenHalfWidthInMeter = (World->TileSideInMeters * (TILE_COUNT_X))/2.0f;
    r32 ScreenHalfHeightInMeter = (World->TileSideInMeters * (TILE_COUNT_Y))/2.0f;

    world_position_difference CameraPlayerDiff = 
            WorldPositionDifferenceInMeter(World, &State->CameraPos, &State->Player->WorldP);
    if(CameraPlayerDiff.P.X > ScreenHalfWidthInMeter)
    {
        State->CameraPos.P.X += 2.0f*ScreenHalfWidthInMeter;
    }
    else if(CameraPlayerDiff.P.X < -ScreenHalfWidthInMeter)
    {
        State->CameraPos.P.X -= 2.0f*ScreenHalfWidthInMeter;
    }
    if(CameraPlayerDiff.P.Y > ScreenHalfHeightInMeter)
    {
        State->CameraPos.P.Y += 2.0f*ScreenHalfHeightInMeter;
    }
    else if(CameraPlayerDiff.P.Y < -ScreenHalfHeightInMeter)
    {
        State->CameraPos.P.Y -= 2.0f*ScreenHalfHeightInMeter;
    }
    CanonicalizeWorldPos(World, &State->CameraPos);

    r32 CenterXInMeter = State->CameraPos.AbsTileX*World->TileSideInMeters + State->CameraPos.P.X;
    r32 CenterYInMeter = State->CameraPos.AbsTileY*World->TileSideInMeters + State->CameraPos.P.Y;

    u32 MinScreenAbsTileX = ConvertMeterToTileCount(World, CenterXInMeter - ScreenHalfWidthInMeter);
    u32 MaxScreenAbsTileX = ConvertMeterToTileCount(World, CenterXInMeter + ScreenHalfWidthInMeter);
    u32 MinScreenAbsTileY = ConvertMeterToTileCount(World, CenterYInMeter - ScreenHalfHeightInMeter);
    u32 MaxScreenAbsTileY = ConvertMeterToTileCount(World, CenterYInMeter + ScreenHalfHeightInMeter);

    for(u32 EntityIndex = 0;
        EntityIndex < State->EntityCount;
        ++EntityIndex)
    {
        low_entity *Entity = State->Entities + EntityIndex;
        if(Entity->WorldP.AbsTileX >= MinScreenAbsTileX && 
            Entity->WorldP.AbsTileY >= MinScreenAbsTileY && 
            Entity->WorldP.AbsTileX <= MaxScreenAbsTileX && 
            Entity->WorldP.AbsTileY <= MaxScreenAbsTileY)
        {
            tile_chunk *TileChunk = GetTileChunk(World, Entity->WorldP.AbsTileX, Entity->WorldP.AbsTileY);

            r32 CenterRelX = Entity->WorldP.AbsTileX*World->TileSideInMeters + Entity->WorldP.P.X - CenterXInMeter;
            r32 CenterRelY = Entity->WorldP.AbsTileY*World->TileSideInMeters + Entity->WorldP.P.Y - CenterYInMeter;
            r32 PixelX = Buffer->Width/2 + CenterRelX*MetersToPixels - 0.5f*MetersToPixels*Entity->Width;
            r32 PixelY = Buffer->Height/2 + CenterRelY*MetersToPixels - 0.5f*MetersToPixels*Entity->Height;
            u32 EntityWidthInPixel = Entity->Width*MetersToPixels;
            u32 EntityHeightInPixel = Entity->Height*MetersToPixels;

            switch(Entity->Type)
            {
                case Entity_Type_Wall:
                {
                    DrawRectangle(Buffer, PixelX, PixelY, 
                                EntityWidthInPixel, EntityHeightInPixel, 
                                1.0f, 1.0f, 1.0f);
                }break;

                case Entity_Type_Player: 
                {
#if 1
                    DrawRectangle(Buffer, PixelX, PixelY, 
                                MetersToPixels*State->Player->Width, MetersToPixels*State->Player->Height, 
                                0.0f, 0.8f, 1.0f);
#else
                    DrawBMP(Buffer, &State->HeadBMP, PixelX, PixelY, 0, 0, State->HeadBMP.AlignmentX, State->HeadBMP.AlignmentY);
                    DrawBMP(Buffer, &State->TorsoBMP, PixelX, PixelY, 0, 0, State->TorsoBMP.AlignmentX, State->TorsoBMP.AlignmentY);
                    DrawBMP(Buffer, &State->CapeBMP, PixelX, PixelY, 0, 0, State->CapeBMP.AlignmentX, State->CapeBMP.AlignmentY);
#endif
                }break;
            }

        }
    }
#if 0

    for(u32 Y = MinScreenAbsTileY;
        Y < MaxScreenAbsTileY;
        ++Y)
    {
        // NOTE : Tile position 0, 0 will be drawn at the EXACT CENTER of the screen.
        r32 CenterRelY = (Y*World->TileSideInMeters - CenterYInMeter);
        r32 PixelY = Buffer->Height/2 + CenterRelY*MetersToPixels - 0.5f*World->TileSideInMeters*MetersToPixels;
        for(u32 X = MinScreenAbsTileX;
            X < MaxScreenAbsTileX;
            ++X)
        {
            tile_chunk *TileChunk = GetTileChunk(World, X, Y);
            if(TileChunk)
            {
                r32 CenterRelX = (X*World->TileSideInMeters - CenterXInMeter);
                r32 PixelX = Buffer->Width/2 + CenterRelX*MetersToPixels - 0.5f*World->TileSideInMeters*MetersToPixels;

                r32 Gray = 1.0f;

                if(GetTileValueFromTileChunkUnchecked(TileChunk, X, Y))
                {
                    Gray = 1.0f;
                }
                else
                {
                    Gray = 0.3f;
                }

                if(State->PlayerPos.AbsTileX == X && State->PlayerPos.AbsTileY == Y)
                {
                    Gray = 0.1f;
                }

                DrawRectangle(Buffer, PixelX, PixelY, 
                            1.0f*World->TileSideInMeters*MetersToPixels,1.0f*World->TileSideInMeters*MetersToPixels, 
                            Gray, Gray, Gray);
            }
        }
    }


    r32 PlayerCenterRelX = State->PlayerPos.AbsTileX*World->TileSideInMeters + State->PlayerPos.X - CenterXInMeter;
    r32 PlayerCenterRelY = State->PlayerPos.AbsTileY*World->TileSideInMeters + State->PlayerPos.Y - CenterYInMeter;
    r32 PlayerPixelX = Buffer->Width/2 + PlayerCenterRelX*MetersToPixels - 0.5f*MetersToPixels*PlayerDim.X;
    r32 PlayerPixelY = Buffer->Height/2 + PlayerCenterRelY*MetersToPixels - 0.5f*MetersToPixels*PlayerDim.Y;

    DrawRectangle(Buffer, PlayerPixelX, PlayerPixelY, MetersToPixels*PlayerDim.X, MetersToPixels*PlayerDim.Y, 0.2, 1.0f, 1.0f);
    DrawRectangle(Buffer, Buffer->Width/2 - 5, Buffer->Height/2 - 5, 10, 10, 1.0, 0.0f, 0.0f);
    //DrawRectangle(Buffer, PlayerTileMapRelX*MetersToPixels, PlayerTileMapRelY*MetersToPixels, MetersToPixels*PlayerDim.X*0.2f, MetersToPixels*PlayerDim.Y*0.2f, 1.0, 0.0f, 0.0f);
#endif

    //DrawBMP(Buffer, &State->SampleBMP, PlayerPixelX, PlayerPixelY);
    //DrawBMP(Buffer, &State->HeadBMP, PlayerPixelX, PlayerPixelY, 0, 0, State->HeadBMP.AlignmentX, State->HeadBMP.AlignmentY);
    //DrawBMP(Buffer, &State->TorsoBMP, PlayerPixelX, PlayerPixelY, 0, 0, State->TorsoBMP.AlignmentX, State->TorsoBMP.AlignmentY);
    //DrawBMP(Buffer, &State->CapeBMP, PlayerPixelX, PlayerPixelY, 0, 0, State->CapeBMP.AlignmentX, State->CapeBMP.AlignmentY);
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
