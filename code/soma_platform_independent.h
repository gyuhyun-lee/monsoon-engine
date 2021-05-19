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
typedef int32 bool32;

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

#define global_variable static
#define local_variable static
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

// TODO : Unform support for all types of controllers?
struct game_controller
{   
    b32 Connected;

    bool AButton;
    bool BButton;
    bool XButton;
    bool YButton;

    bool DPadRight;
    bool DPadLeft;
    bool DPadUp;
    bool DPadDown;

    // TODO :Collaps these values into vector
    // NOTE : These are -1.0f ~ 1.0f value
    r32 LeftStickX;
    r32 LeftStickY;
    r32 RightStickX;
    r32 RightStickY;

    b32 RightShoulder;
        
    b32 LeftShoulder;
    b32 RightTrigger;
    b32 LeftTrigger;

    b32 MinusButton;
    b32 CaptureButton;
    b32 PlusButton;
    b32 HomeButton;
};

struct game_input
{
    game_controller Controllers[4];
    u32 ControllerCount;
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

#define GAME_UPDATE_AND_RENDER(name) void (name)(game_offscreen_buffer *OffscreenBuffer, game_memory *Memory, game_platform_api *PlatformAPI, r32 dtPerFrame)
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

struct game_state
{
    int a = 1;
};

#endif
