#include "soma_platform_independent.h"

internal void 
RenderSomething(game_offscreen_buffer *Buffer)
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
            if(Column == Buffer->Width/2 ||
            Row == Buffer->Height/2)
            {
                *Pixel++ = (0xffff00ff);
            }
            else
            {
                *Pixel++ = (0xffffff00);
            }
        }

        FirstPixelOfRow += Buffer->Pitch;
    }
}

internal void
FillAudioBuffer(game_audio_buffer *AudioBuffer)
{
    // TODO : Sine wave here
    u32 T = 1.0f;
    u32 RunningSampleIndex = AudioBuffer->WriteSampleIndex;
    u32 SamplesToFillCountPad = 160;
    // TODO : Use the actual dt here to calculate how much do I want to fill
    u32 SamplesToFillCount = ((1/30.0f) * AudioBuffer->SamplesPerSecond) + SamplesToFillCountPad;

    u32 SquareWavePeriod = (AudioBuffer->SamplesPerSecond/256);
    u32 HalfSquareWavePeriod = SquareWavePeriod / 2;

    u32 Volume = 3000;

    for(u32 SampleIndex = 0;
        SampleIndex < SamplesToFillCount;
        ++SampleIndex)
    {
        i16 LeftSampleValue = 1;
        i16 RightSampleValue = 1;
        if(RunningSampleIndex%SquareWavePeriod > HalfSquareWavePeriod)
        {
            i16 LeftSampleValue = -1;
            i16 RightSampleValue = -1;
        }
        AudioBuffer->Samples[RunningSampleIndex++] = LeftSampleValue * Volume;
        AudioBuffer->Samples[RunningSampleIndex++] = RightSampleValue * Volume;

        RunningSampleIndex %= AudioBuffer->SampleCount;
    }

    AudioBuffer->WriteSampleIndex = RunningSampleIndex;
}

extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    RenderSomething(OffscreenBuffer);

    FillAudioBuffer(AudioBuffer);
}
