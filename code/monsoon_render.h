#ifndef MONSOON_RENDER_H
#define MONSOON_RENDER_H

// NOTE : This buffer is a generic pixel buffer with each pixel being 32 bit value, 
// and will be used for the FinalBuffer before the screen buffer,
// background buffer...etc, NOT the offscreen buffer
struct pixel_buffer_32
{
    // NOTE : Make sure these are int, not uint.
    i32 width;
    i32 height;
    i32 pitch;

    u32 bytesPerPixel;

    v2 alignment;
    
    u32 *memory;
};

struct environment_map
{
    v3 P; // TODO: Maybe we should store a world position
    v2 dim;
    pixel_buffer_32 LOD;
};

struct debug_loaded_bmp
{
    game_offscreen_buffer BMP;
    v2 Alignment;
};

enum render_element_type
{
    RenderElementType_Rect,
    RenderElementType_BMP,
};

struct render_element_header
{
    render_element_type Type;
    // TODO : This should be v3 so can we can sort using Z
    v2 p;
    v4 color;
    v2 xAxis;
    v2 yAxis;
};

struct render_element_rect
{
};

struct render_element_bmp
{
    // TODO : Do not store the whole BMP info, fix this when the game has compressed asset
    // Maybe store a pointer to the memory_loaded asset?
    pixel_buffer_32 *sourceBuffer;

    // TODO : Do we even need this alignment value?
    // because we always starts at p - halfDim to p + halfdim, and
    // the bmp will just fill up the space that we requested.
    // If it turns out that we need this alignment value, we have to adjust
    // the value based on the dim?
    v2 alignment;

    environment_map *envMaps;
    pixel_buffer_32 *normalMap;
};

struct render_group
{
    temporary_memory renderMemory;

    u32 elementCount;
    r32 metersToPixels;

    v2 bufferHalfDim;
};

internal void PushBMP(render_group *renderGroup, pixel_buffer_32 *sourceBuffer, v3 p, v3 dim, 
                    v2 xAxis, v2 yAxis, 
                    pixel_buffer_32 *normalMap, environment_map *envMaps);
internal void
PushRect(render_group *renderGroup, v3 p, v3 dim, v3 color, v2 xAxis, v2 yAxis);

#endif
