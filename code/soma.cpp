#include "soma_platform_independent.h"
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

extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) < Memory->PermanentStorageSize);

    game_state *State = (game_state *)Memory->PermanentStorage;

    State->a = 1;

    RenderSomething(OffscreenBuffer);
    debug_platform_read_file_result File = PlatformAPI->DEBUGReadEntireFile("/Volumes/work/soma/data/sample.bmp");
    PlatformAPI->DEBUGWriteEntireFile("/Volumes/work/soma/data/sample_temp.bmp", File.Memory, File.Size);
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
