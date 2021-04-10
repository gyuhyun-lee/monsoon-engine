/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#ifndef FOX_PLATFORM_INDEPENDENT_H

#include <stdint.h>
#include <limits.h>
#include <float.h>

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

// TODO : Check these work on all computers!(32bit vs 64bit)
#define i32_min INT_MIN
#define i32_max INT_MAX
#define u32_max UINT_MAX
#define r32_max FLT_MAX
#define r64_max DBL_MAX

#define global_variable static
#define internal static

#define Bytes_Per_Pixel 4

#define Pi32 3.141592653589f
#define HalfPi32 1.5707963267945
#define Tau32 6.28318530718f
#define Rad1 0.0174533f

inline r32 
Rad(r32 angleInDegree)
{
    return angleInDegree * 0.0174533f;
}

inline i32
RoundDownr32Toi32(r32 value)
{
	return (i32)(value - 1.0f);
}

// inline i32
// Roundr32Toi32(r32 value)
// {
// 	if(value >= 0.0f)
// 	{

// 	}
// }

#define Assert(expression) if(!(expression)) {int *a = 0; *a = 1;}
#define Assertifnot(expression) Assert(expression)
#define InvalidCodePath Assert(0)
#define Deprecated Assert(0)
#define DefaultCase default:{}break

#define Kilobytes(size) 1024 * size
#define Megabytes(size) 1024 * Kilobytes(size)
#define Gigabytes(size) 1024 * Megabytes(size)

#define ArrayCount(array) sizeof(array)/sizeof(*array)

struct v2
{
	union
	{
		struct
		{
			r32 x;
			r32 y;
		};

		r32 e[2];
	};
};

struct v2i
{
	union
	{
		struct
		{
			i32 x;
			i32 y;
		};

		i32 e[2];
	};
};

struct v3
{
	union
	{
		struct
		{
			r32 x;
			r32 y;
			r32 z;
		};

		struct
		{
			v2 xy;
			r32 ignored0;
		};
		struct
		{
			r32 ignored1;
			v2 yz;
		};
		struct 
		{
			r32 r;
			r32 g;
			r32 b;
		};

		r32 e[3];
	};
};

struct v3i
{
	union
	{
		struct
		{
			i32 x;
			i32 y;
			i32 z;
		};

		struct
		{
			v2i xy;
			i32 ignored0;
		};
		struct
		{
			i32 ignored1;
			v2i yz;
		};
		struct 
		{
			i32 r;
			i32 g;
			i32 b;
		};

		i32 e[3];
	};
};

struct v3u
{
	union
	{
		struct
		{
			u32 x;
			u32 y;
			u32 z;
		};

		// struct
		// {
		// 	v2i xy;
		// 	i32 ignored0;
		// };
		// struct
		// {
		// 	i32 ignored1;
		// 	v2i yz;
		// };
		// struct 
		// {
		// 	i32 r;
		// 	i32 g;
		// 	i32 b;
		// };

		u32 e[3];
	};
};

struct v4
{
	union
	{
		struct
		{
			r32 x;
			r32 y;
			r32 z;
			r32 w;
		};

		struct
		{
			v3 xyz;
			r32 ignored0;
		};
		struct 
		{
			r32 r;
			r32 g;
			r32 b;
			r32 a;
		};
		struct
		{
			v3 rgb;
			r32 ignored1;
		};

		r32 e[4];
	};
};

struct game_screen_buffer
{
	i32 width;
	i32 height;
	i32 pitch;
    i32 size;
	v2 center; // TODO : Is storing center to the buffer really necessairly?

	void *memory;
};

struct game_audio_buffer
{
    // TODO : Maybe make this just to i16 *samples
    int16 *samples[2];
    i32 bytesPerSample;

    u32 channelCount;

    u32 sampleCount;
    u32 totalBytesPerChannel;
    u32 samplesPerSecond;

    u32 runningSampleIndex;
};

struct game_button
{
	b32 isDown;
	b32 wasDown;
};

struct game_controller
{
	v2 mouseP; // IMPORTANT : This range from -1 ~ 1, screen center is 0, 0

	// NOTE : These are based on game controller
	v2 leftStick;
	v2 rightStick;


	r32 leftTrigger;
	r32 rightTrigger;

	union
	{
		struct
		{
			game_button actionDown;
			game_button actionUp;
			game_button actionLeft;
			game_button actionRight;

			game_button dPadRight;
			game_button dPadLeft;
			game_button dPadUp;
			game_button dPadDown;

			game_button leftBumper;
			game_button rightBumper;

			game_button back;
			game_button start;
		};

		// NOTE : Make sure to increment this whenever I add new game_button!
		game_button buttons[12];
	};
};

struct game_input
{
	// TODO : Might increase the number of controllers that the game can support
	/* 
		NOTE : 
		0 : keyboard & gamepad
	*/
	game_controller controllers[2];
};

#define memory_index size_t
// TODO : How to support multiple memory arena?
// TODO : Proper freeing memory!
struct memory_arena
{
	u8 *base;

	memory_index totalSize;
	memory_index used;

	i32 tempCount;
};

inline memory_arena 
StartMemoryArena(u8 *memory, memory_index totalSize)
{
	memory_arena result = {};

	result.base = memory;
	result.totalSize = totalSize;
	result.used = 0;
	result.tempCount = 0;

	return result;
}

inline void
CheckMemoryArenaTempCount(memory_arena *memoryArena)
{
	Assert(memoryArena->tempCount == 0);
}

#define PushStruct(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, type, count) PushSize_(arena, sizeof(type) * count)

inline void *
PushSize_(memory_arena *arena, memory_index size)
{
	// TODO : Alignment
	void *result = arena->base + arena->used;

	arena->used += size;
	Assert(arena->used <= arena->totalSize);

	return result;
}

// NOTE : These _SHOULD ONLY_ persist for _ONE LOOP_
// Otherwise, I will have memory leak!
struct temporary_memory
{
	memory_arena *memoryArena;

	u8 *base;
	memory_index totalSize;
	memory_index used;
};

inline temporary_memory
StartTemporaryMemory(memory_arena *memoryArena, memory_index totalSize)
{
	temporary_memory result = {};
	// TODO : There's no safe bound checking here!!
	result.memoryArena = memoryArena;
	result.base = memoryArena->base + memoryArena->used;
	result.totalSize = totalSize;
	result.used = 0;

	memoryArena->tempCount++;
	memoryArena->used += totalSize;
	Assert(memoryArena->used <= memoryArena->totalSize);

	return result;
}

inline u8 *
AllocateTemporaryMemory(temporary_memory *temporaryMemory, memory_index size)
{
	u8 *result = temporaryMemory->base + temporaryMemory->used;
	temporaryMemory->used += size;
	return result;
}

#define TemporaryMemory(temporaryMemory, type) (type *)GetTemporaryMemory(temporaryMemory, sizeof(type))

inline void
EndTemporaryMemory(temporary_memory *temporaryMemory)
{
	memory_arena *memoryArena = temporaryMemory->memoryArena;

	memoryArena->tempCount--;
	// TODO : This is also risky because it only decrease the total used size,
	// and _DOES NOT FREE THE SPECIFIC MEMORY SPACE_.
	memoryArena->used -= temporaryMemory->totalSize;

	// NOTE : Might not be useful 
	// because we always end the memory at the end of the loop
	// *temporaryMemory = {};
}

inline i16
Encode2Bytes(char *letters, b32 isLittleEndian)
{
    i16 result = 0;

    if(isLittleEndian)
    {
        result = (letters[0] << 8)|(letters[1]);
    }
    else
    {
        result = (letters[1] << 8)|(letters[0]);
    }

    return result;
}

inline u32
Encode4Bytes(char *letters, b32 isLittleEndian)
{
    u32 result = 0;

    if(isLittleEndian)
    {
        result = (letters[0] << 24)|(letters[1]<<16)|(letters[2]<<8)|(letters[3]);
    }
    else
    {
        result = (letters[3] << 24)|(letters[2]<<16)|(letters[1]<<8)|(letters[0]);
    }

    return result;
}

struct game_memory
{
	void *permanentStorage;
	u64 permanentStorageSize;

	void *transientStorage;
	u64 transientStorageSize;
};

#define DEBUG_PLATFORM_READ_FILE(name) void *name(char *filePath)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) b32 name(char *filePath, void *source, memory_index sourceSize)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

#define DEBUG_PLATFORM_FREE_FILE(name) void name(void *pointer)
typedef DEBUG_PLATFORM_FREE_FILE(debug_platform_free_file);

struct platform_api
{
	debug_platform_read_file *PlatformReadFile;
	debug_platform_write_file *PlatformWriteFile;
	debug_platform_free_file *PlatformFreeFile;
};
#define GAME_UPDATE_AND_RENDER(name) void name(game_screen_buffer *gameBuffer, game_input *gameInput, game_input *oldInput, game_memory *gameMemory, platform_api *platformAPI, r32 targetSecondsPerFrame)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_UPDATE_AUDIO_BUFFER(name) void name(game_audio_buffer *gameAudioBuffer, game_memory *gameMemory, r32 dt)
typedef GAME_UPDATE_AUDIO_BUFFER(game_update_audio_buffer);

struct game_code
{
	game_update_and_render *GameUpdateAndRender;
    game_update_audio_buffer *GameUpdateAudioBuffer;
};

#define FOX_PLATFORM_INDEPENDENT_H
#endif
