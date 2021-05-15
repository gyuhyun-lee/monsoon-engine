#ifndef SOMA_PLATFORM_INDEPENDENT
#define SOMA_PLATFORM_INDEPENDENT

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8; 
typedef uint16_t uint16; typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8 i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;
typedef int32 b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;

typedef float r32;
typedef double r64;

#define SecToNanoSec 1.0e+9

#define Assert(Expression) if(!(Expression)) {int *a = 0; *a = 0;}

#define global_variable static
#define local_varible static
#define internal static

struct game_offscreen_buffer
{
    u32 Width;
    u32 Height;
    u32 Pitch;

    u32 BytesPerPixel;
    void *Memory;
};

struct game_audio_buffer
{
    i16 *Samples;
    u32 SampleCount;
    u32 ChannelCount;
    u32 SamplesPerSecond;

    u32 ConsumedSampleIndex; // NOTE : Audio Sample was consumed until this sample index
    u32 WriteSampleIndex; // NOTE : Audio Sample was filled until this sample index
};

#define GAME_UPDATE_AND_RENDER(name) void (name)(game_offscreen_buffer *OffscreenBuffer, game_audio_buffer *AudioBuffer)
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub){}
typedef void (game_update_and_render)(game_offscreen_buffer *OffscreenBuffer, game_audio_buffer *AudioBuffer);

struct game_code
{
    void *Handle;
    u64 LastModifiedTime;
    game_update_and_render *UpdateAndRender;
};

#endif
