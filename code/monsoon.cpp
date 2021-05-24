#include "monsoon_platform_independent.h"
#include "monsoon_intrinsic.h"

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

internal tile_chunk*
GetTileChunk(world *World, u32 TileX, u32 TileY)
{
    tile_chunk *Result = 0;

    // TODO : ArrayCount!
    // TODO : Hashing instead of looping all the tile chunks
    for(u32 TileChunkIndex = 0;
        TileChunkIndex < 4;
        ++TileChunkIndex)
    {
        tile_chunk *TileChunk = World->TileChunks + TileChunkIndex;
        
        u32 MaxAbsTileX = TileChunk->MinAbsTileX + TileChunk->TileCountX;
        u32 MaxAbsTileY = TileChunk->MinAbsTileY + TileChunk->TileCountY;

        if(TileX >= TileChunk->MinAbsTileX && TileX < MaxAbsTileX &&
            TileY >= TileChunk->MinAbsTileY && TileY < MaxAbsTileY)
        {
            Result = TileChunk;
            break;
        }
    }

    return Result;
}

// NOTE : TileX and TileY should be canonicalized
internal u32
GetTileValueUnsafe(u32 *Tiles, u32 TileCountX, u32 TileCountY, u32 TileX, u32 TileY)
{
    Assert(TileX >= 0 && TileY >= 0 && TileX < TileCountX && TileY < TileCountY);
    u32 Result = Tiles[TileCountX * (TileCountY - TileY - 1) + TileX];
    return Result;
}

internal b32 
IsTileEmptyUnsafe(tile_chunk *TileChunk, u32 TileX, u32 TileY)
{
    b32 Result = false;

    u32 TileChunkRelTileX = TileX - TileChunk->MinAbsTileX;
    u32 TileChunkRelTileY = TileY - TileChunk->MinAbsTileY;

    Assert(TileChunkRelTileX < TileChunk->TileCountX);
    Assert(TileChunkRelTileY < TileChunk->TileCountY);

    if(!GetTileValueUnsafe(TileChunk->Tiles, TileChunk->TileCountX, TileChunk->TileCountY, TileChunkRelTileX, TileChunkRelTileY))
    {
        Result = true;
    }

    return Result;
}

internal void
CanonicalizeWorldPos(world *World, world_position *WorldPos)
{
    i32 TileOffsetX = FloorR32ToInt32(WorldPos->X/World->TileSideInMeters);
    i32 TileOffsetY = FloorR32ToInt32(WorldPos->Y/World->TileSideInMeters);

    WorldPos->TileX += TileOffsetX;
    WorldPos->X -= TileOffsetX * World->TileSideInMeters;

    WorldPos->TileY += TileOffsetY;
    WorldPos->Y -= TileOffsetY*World->TileSideInMeters;
}

// NOTE : This function always needs canonicalized position
internal b32
IsWorldPointEmptyUnsafe(world *World, world_position CanPos)
{
    b32 Result = false;

    tile_chunk *TileChunk = GetTileChunk(World, CanPos.TileX, CanPos.TileY);
    if(TileChunk)
    {

        Result = IsTileEmptyUnsafe(TileChunk, CanPos.TileX, CanPos.TileY);
    }

    return Result;
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
        r32 X, r32 Y, r32 Width = 0, r32 Height = 0)
{
    i32 MinX = RoundR32ToInt32(X);
    i32 MinY = RoundR32ToInt32(Y);
    i32 MaxX = RoundR32ToInt32(X + LoadedBMP->Width);
    i32 MaxY = RoundR32ToInt32(Y + LoadedBMP->Height);
    
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

    u8 *Row = (u8 *)Buffer->Memory + 
                    Buffer->Pitch*MinY + 
                    Buffer->BytesPerPixel*MinX;
    u8 *SourceRow = (u8 *)LoadedBMP->Pixels;

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
            *Pixel++ = *SourcePixel++;
        }

        Row += Buffer->Pitch;
        SourceRow += LoadedBMP->Pitch;
    }
}

extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) < Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorage;
    if(!State->IsInitialized)
    {
        State->XOffset = 0;
        State->YOffset = 0;

        State->PlayerPos.X = 0;
        State->PlayerPos.Y = 0;
        State->PlayerPos.TileX = 5;
        State->PlayerPos.TileY = 5;

        State->SampleBMP = DEBUGLoadBMP(PlatformAPI->DEBUGReadEntireFile, "/Volumes/work/soma/data/temp.bmp");

        State->IsInitialized = true;
    }

    // TODO : This should go away!

#define TILE_COUNT_X 17
#define TILE_COUNT_Y 9


    u32 Tiles00[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 0, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 1, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 0},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 Tiles01[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 1, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {0, 0, 0, 0,     0, 1, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
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
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 0, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 Tiles11[TILE_COUNT_Y][TILE_COUNT_X] = 
    {
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 1, 1, 1,     1, 1, 1, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 0,     0, 0, 0, 1, 1},
        {1, 1, 1, 1,     1, 1, 1, 1,     0, 0, 1, 1,     1, 1, 1, 1, 1}
    };

    u32 MinCornerPixelX = 0;
    u32 MinCornerPixelY = 0;

    // TODO : Tile Chunk Construction
    world World = {};
    World.TileSideInMeters = 2.0f;
    World.TileChunks[0].Tiles = (u32 *)Tiles00;
    World.TileChunks[0].MinAbsTileX = 0;
    World.TileChunks[0].MinAbsTileY = 0;
    World.TileChunks[0].TileCountX = TILE_COUNT_X;
    World.TileChunks[0].TileCountY = TILE_COUNT_Y;

    World.TileChunks[1].MinAbsTileX = TILE_COUNT_X;
    World.TileChunks[1].MinAbsTileY = 0;
    World.TileChunks[1].Tiles = (u32 *)Tiles01;
    World.TileChunks[1].TileCountX = TILE_COUNT_X;
    World.TileChunks[1].TileCountY = TILE_COUNT_Y;


    World.TileChunks[2].MinAbsTileX = 0;
    World.TileChunks[2].MinAbsTileY = TILE_COUNT_Y;
    World.TileChunks[2].Tiles = (u32 *)Tiles10;
    World.TileChunks[2].TileCountX = TILE_COUNT_X;
    World.TileChunks[2].TileCountY = TILE_COUNT_Y;

    World.TileChunks[3].MinAbsTileX = TILE_COUNT_X;
    World.TileChunks[3].MinAbsTileY = TILE_COUNT_Y;
    World.TileChunks[3].Tiles = (u32 *)Tiles11;
    World.TileChunks[3].TileCountX = TILE_COUNT_X;
    World.TileChunks[3].TileCountY = TILE_COUNT_Y;
    r32 PixelToMeters = 0.02f;
    r32 MetersToPixels = 1.0f/PixelToMeters;

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

    r32 PlayerWidth = 0.5f*World.TileSideInMeters;
    r32 PlayerHeight = World.TileSideInMeters;
    r32 PlayerSpeedX = 5.0f;
    r32 PlayerSpeedY = 5.0f;

    if(Input.ActionRight)
    {
        PlayerSpeedX = 20.f;
        PlayerSpeedY = 20.f;
    }

    // TODO : Change this to dPlayer
    world_position NewPlayerPos = State->PlayerPos;

    NewPlayerPos.X = State->PlayerPos.X + dtPerFrame * PlayerSpeedX * (Input.MoveRight - Input.MoveLeft);
    NewPlayerPos.Y = State->PlayerPos.Y + dtPerFrame * PlayerSpeedY * (Input.MoveUp - Input.MoveDown);
    
    world_position NewPlayerRightPos = NewPlayerPos;
    NewPlayerRightPos.X += PlayerWidth;

    CanonicalizeWorldPos(&World, &NewPlayerPos);
    CanonicalizeWorldPos(&World, &NewPlayerRightPos);

    if(IsWorldPointEmptyUnsafe(&World, NewPlayerPos) &&
        IsWorldPointEmptyUnsafe(&World, NewPlayerRightPos))
    {
        State->PlayerPos = NewPlayerPos;
    }

    ClearBuffer(Buffer);

    tile_chunk *PlayerTileChunk = GetTileChunk(&World, State->PlayerPos.TileX, State->PlayerPos.TileY);
    for(u32 Y = 0;
        Y < TILE_COUNT_Y;
        ++Y)
    {
        u32 PixelY = MinCornerPixelY + Y*World.TileSideInMeters*MetersToPixels;
        for(u32 X = 0;
            X < TILE_COUNT_X;
            ++X)
        {
            u32 PixelX = MinCornerPixelX + X*World.TileSideInMeters*MetersToPixels;
            if(GetTileValueUnsafe(PlayerTileChunk->Tiles, TILE_COUNT_X, TILE_COUNT_Y, X, Y))

            {
                DrawRectangle(Buffer, PixelX, PixelY, 
                                0.9f*World.TileSideInMeters*MetersToPixels, 0.9f*World.TileSideInMeters*MetersToPixels, 
                                1.0f, 1.0f, 1.0f);
            }
            else
            {
                DrawRectangle(Buffer, PixelX, PixelY, 
                            0.9f*World.TileSideInMeters*MetersToPixels, 0.9f*World.TileSideInMeters*MetersToPixels, 
                            0.3f, 0.3f, 0.3f);
            }
        }
    }

    u32 TileChunkRelTileX = State->PlayerPos.TileX - PlayerTileChunk->MinAbsTileX;
    u32 TileChunkRelTileY = State->PlayerPos.TileY - PlayerTileChunk->MinAbsTileY;

    r32 PlayerTileMapRelX = World.TileSideInMeters * TileChunkRelTileX + State->PlayerPos.X;
    r32 PlayerTileMapRelY = World.TileSideInMeters * TileChunkRelTileY + State->PlayerPos.Y;

    DrawRectangle(Buffer, PlayerTileMapRelX*MetersToPixels, PlayerTileMapRelY*MetersToPixels, MetersToPixels*PlayerWidth, MetersToPixels*PlayerHeight, 0.2, 1.0f, 1.0f);
    DrawRectangle(Buffer, PlayerTileMapRelX*MetersToPixels, PlayerTileMapRelY*MetersToPixels, MetersToPixels*PlayerWidth*0.2f, MetersToPixels*PlayerHeight*0.2f, 1.0, 0.0f, 0.0f);

    DrawBMP(Buffer, &State->SampleBMP, PlayerTileMapRelX*MetersToPixels, PlayerTileMapRelY*MetersToPixels, MetersToPixels*PlayerWidth*0.2f, MetersToPixels*PlayerHeight*0.2f);
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
