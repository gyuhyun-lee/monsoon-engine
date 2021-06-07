#ifndef SOMA_PLATFORM_INDEPENDENT
#define SOMA_PLATFORM_INDEPENDENT

#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 b3232;

typedef uint8_t uint8; 
typedef uint16_t uint16; 
typedef uint32_t uint32;
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

#define Kilobytes(x) 1024LL*x
#define Megabytes(x) 1024LL*Kilobytes(x)
#define Gigabytes(x) 1024LL*Megabytes(x)
#define Terabytes(x) 1024LL*Megabytes(x)

#define Pi32 3.141592653589793238462643383279502884197169399375105820974944592307816406286f

#define SecToNanoSec 1.0e+9

#define Assert(Expression) if(!(Expression)) {int *a = 0; *a = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define Minimum(a, b) ((a < b) ? a : b)
#define Maximum(a, b) ((a > b)? a : b)

#define global_variable static
#define local_variable static
#define internal static

#include "monsoon_math.h" 

struct game_offscreen_buffer
{
    i32 Width;
    i32 Height;

    i32 Pitch;

    u32 BytesPerPixel;
    void *Memory;
};

struct game_audio_buffer
{
    // NOTE : Current samples are LEFT RIGHT LEFT RIGHT and each channel is 16 bit.
    // TODO : Change this to non-interleaved form to avoid confusion?
    i16 *Samples;
    u32 SampleCount;
    u32 ChannelCount;
    u32 SamplesPerSecond;

    u32 ConsumedSampleIndex; // NOTE : Audio Sample was consumed until this sample index
    u32 RunningSampleIndex;

    b32 IsSoundReady;
};

struct game_controller
{   
    b32 IsAnalog;

    // TODO :Collaps these values into vector
    // NOTE : These are -1.0f ~ 1.0f value
    r32 LeftStickX;
    r32 LeftStickY;
    r32 RightStickX;
    r32 RightStickY;

    b32 MoveUp;
    b32 MoveDown;
    b32 MoveLeft;
    b32 MoveRight;

    b32 AButton;
    b32 BButton;
    b32 XButton;
    b32 YButton;

    b32 DPadRight;
    b32 DPadLeft;
    b32 DPadUp;
    b32 DPadDown;

    b32 RightShoulder;
    b32 RightTrigger;
    b32 LeftShoulder;
    b32 LeftTrigger;

    b32 MinusButton;
    b32 CaptureButton;
    b32 PlusButton;
    b32 HomeButton;
};

struct game_input_raw
{
    game_controller Controllers[4];
    u32 ControllerCount;
};

struct game_input_manager
{
    game_input_raw RawInputs[2];
    u32 NewInputIndex; // NOTE : This value is 0 or 1, and will be changed at the end of main loop.
};

struct game_input
{   
    b32 MoveUp;
    b32 MoveDown;
    b32 MoveLeft;
    b32 MoveRight;

    b32 ActionRight;
};

struct game_memory
{
    void *PermanentStorage;
    void *TransientStorage;

    u64 PermanentStorageSize;
    u64 TransientStorageSize;
};

struct debug_platform_read_file_result
{
    void *Memory;
    u64 Size;
};
#define DEBUG_PLATFORM_READ_FILE(name) debug_platform_read_file_result (name)(char *FileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_read_entire_file);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) void (name)(char *FileNameToCreate, void *MemoryToWrite, u64 FileSize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_write_entire_file);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *MemoryToFree)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_free_file_memory);
struct game_platform_api
{
    debug_read_entire_file *DEBUGReadEntireFile;
    debug_write_entire_file *DEBUGWriteEntireFile;
    debug_free_file_memory *DEBUGFreeFileMemory;
};

#define GAME_UPDATE_AND_RENDER(name) void (name)(game_offscreen_buffer *Buffer, game_memory *Memory, game_input_raw *RawInput, game_platform_api *PlatformAPI, r32 dtPerFrame)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub){}

#define GAME_FILL_AUDIO_BUFFER(name) void (name)(game_audio_buffer *AudioBuffer, r32 dtPerFrame)
typedef GAME_FILL_AUDIO_BUFFER(game_fill_audio_buffer);
GAME_FILL_AUDIO_BUFFER(GameFillAudioBufferStub){}

struct game_code
{
    void *Handle;
    u64 LastModifiedTime;

    game_update_and_render *UpdateAndRender;
    game_fill_audio_buffer *FillAudioBuffer;
};

struct debug_loaded_bmp
{
    u32 Width;
    u32 Height;
    u32 Pitch;
    u32 BytesPerPixel;

    r32 AlignmentX;
    r32 AlignmentY;

    u32 *Pixels;
};

struct debug_game_input_record
{
    /*NOTE : 
     *  Input record starts with 20 seconds worth of memory, 
     *  and reallocates twice as much memory when the memory is depleted.
     *  Input record memory first comes with the initial game_state,
     *  followed by each frame worth of game_input
    */
    void *Memory;
    u32 MemorySize;

    u32 PlayIndex;
    u32 PlayIndexCount;
    u32 MaxPlayIndexCount;

    b32 IsRecording;
    b32 IsPlaying;
};

#define memory_index size_t
struct memory_arena
{
    memory_index TotalSize;
    memory_index Used;

    u8 *Base;
};

inline u8 *
PushSize(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->TotalSize);

    u8 *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, count) (type *)PushSize(Arena, sizeof(type)*count)

internal memory_arena
StartMemoryArena(u8 *Base, memory_index TotalSize)
{
    memory_arena Result = {};

    Result.Base = Base;
    Result.TotalSize = TotalSize;

    return Result;
}

#endif
