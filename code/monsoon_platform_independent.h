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
    i32 width;
    i32 height;

    i32 pitch;

    u32 bytesPerPixel;
    void *memory;
};

struct game_audio_buffer
{
    // NOTE : Current samples are LEFT RIGHT LEFT RIGHT and each channel is 16 bit.
    // TODO : Change this to non-interleaved form to avoid confusion?
    i16 *samples;
    u32 sampleCount;
    u32 channelCount;
    u32 samplesPerSecond;

    u32 consumedSampleIndex; // NOTE : Audio Sample was consumed until this sample index
    u32 runningSampleIndex;

    b32 isSoundReady;
};

struct game_controller
{   
    b32 isAnalog;

    // TODO :Collaps these values into vector
    // NOTE : These are -1.0f ~ 1.0f value
    r32 leftStickX;
    r32 leftStickY;
    r32 rightStickX;
    r32 rightStickY;

    b32 moveUp;
    b32 moveDown;
    b32 moveLeft;
    b32 moveRight;

    b32 AButton;
    b32 BButton;
    b32 XButton;
    b32 YButton;

    b32 DPadRight;
    b32 DPadLeft;
    b32 DPadUp;
    b32 DPadDown;

    b32 rightShoulder;
    b32 rightTrigger;
    b32 leftShoulder;
    b32 leftTrigger;

    b32 minusButton;
    b32 captureButton;
    b32 plusButton;
    b32 homeButton;
};

struct game_input_raw
{
    game_controller controllers[4];
    u32 controllerCount;
};

struct game_input_manager
{
    game_input_raw rawInputs[2];
    u32 newInputIndex; // NOTE : This value is 0 or 1, and will be changed at the end of main loop.
};

struct game_input
{   
    b32 moveUp;
    b32 moveDown;
    b32 moveLeft;
    b32 moveRight;

    b32 actionRight;
};

struct game_memory
{
    void *permanentStorage;
    void *transientStorage;

    u64 permanentStorageSize;
    u64 transientStorageSize;
};

struct debug_platform_read_file_result
{
    void *memory;
    u64 size;
};
#define DEBUG_PLATFORM_READ_FILE(name) debug_platform_read_file_result (name)(char *fileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_read_entire_file);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) void (name)(char *fileNameToCreate, void *memoryToWrite, u64 fileSize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_write_entire_file);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memoryToFree)
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

#define GAME_FILL_AUDIO_BUFFER(name) void (name)(game_audio_buffer *audioBuffer, r32 dtPerFrame)
typedef GAME_FILL_AUDIO_BUFFER(game_fill_audio_buffer);
GAME_FILL_AUDIO_BUFFER(GameFillAudioBufferStub){}

struct game_code
{
    void *handle;
    u64 lastModifiedTime;

    game_update_and_render *UpdateAndRender;
    game_fill_audio_buffer *FillAudioBuffer;
};

struct debug_game_input_record
{
    /*NOTE : 
     *  Input record starts with 20 seconds worth of memory, 
     *  and reallocates twice as much memory when the memory is depleted.
     *  Input record memory first comes with the initial game_state,
     *  followed by each frame worth of game_input
    */
    void *memory;
    u32 memorySize;

    u32 playIndex;
    u32 playIndexCount;
    u32 maxPlayIndexCount;

    b32 isRecording;
    b32 isPlaying;
};

#define memory_index size_t
struct memory_arena
{
    memory_index totalSize;
    memory_index used;

    u32 temporaryMemoryCount;

    u8 *base;
};

inline u8 *
PushSize(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->used + Size) <= Arena->totalSize);

    u8 *Result = Arena->base + Arena->used;
    Arena->used += Size;

    return Result;
}

struct temporary_memory
{
    memory_arena *arena;

    memory_index totalSize;
    memory_index used;

    u8 *base;
};

internal temporary_memory
StartTemporaryMemory(memory_arena *Arena, memory_index totalSize)
{
    temporary_memory TemporaryMemory = {};

    TemporaryMemory.arena = Arena;
    TemporaryMemory.totalSize = totalSize;
    TemporaryMemory.base = PushSize(TemporaryMemory.arena, TemporaryMemory.totalSize);

    TemporaryMemory.arena->temporaryMemoryCount++;

    return TemporaryMemory;
}

internal u8 *
PushSize(temporary_memory *TemporaryMemory, memory_index Size)
{
    Assert((TemporaryMemory->used + Size) <= TemporaryMemory->totalSize);

    u8 *Result = TemporaryMemory->base + TemporaryMemory->used;
    TemporaryMemory->used += Size;

    return Result;
}

internal void
EndTemporaryMemory(temporary_memory TemporaryMemory)
{
    memory_arena *Arena = TemporaryMemory.arena;
    Arena->temporaryMemoryCount--;
    Arena->used -= TemporaryMemory.totalSize;
}

internal void
CheckMemoryArenaTemporaryMemory(memory_arena *Arena)
{
    Assert(Arena->temporaryMemoryCount == 0);
}


#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, count) (type *)PushSize(Arena, sizeof(type)*count)

internal memory_arena
StartMemoryArena(u8 *base, memory_index totalSize)
{
    memory_arena Result = {};

    Result.base = base;
    Result.totalSize = totalSize;

    return Result;
}

#endif
