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
    r32 widthOverHeight;
    i32 pitch;

    u32 bytesPerPixel;
    v2 alignPercentage;
    
    u32 *memory;
};

struct environment_map
{
    v3 P; // TODO: Maybe we should store a world position
    v2 dim;
    pixel_buffer_32 LOD;
};

enum render_element_type
{
    RenderElementType_Rect,
    RenderElementType_BMP,
};

struct render_element_header
{
    render_element_type type;
    // TODO : This should be v3 so can we can sort using Z
    v3 p;
    v3 dim;

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

    // TODO : This is also in meters, and the p will be subtract this value from itself
    // before transforming into perspective p. Don't know the calculation is correct : double-check!
    v2 alignment;

    environment_map *envMaps;
    pixel_buffer_32 *normalMap;
};

struct render_group_camera
{
    r32 z;
    r32 focalLength;
    v2 projectedMonitorDim;
    r32 metersToPixels;
};

struct render_group
{
    // NOTE : Camera values, in meters
    render_group_camera renderCamera; // camera that will be used for rendering
    render_group_camera gameCamera; // camera that will be used for getting entities
    
    temporary_memory renderMemory;

    u32 elementCount;

    v2 bufferHalfDim;
};

internal void PushBMP(render_group *renderGroup, pixel_buffer_32 *sourceBuffer, v3 p, v3 dim, 
                    v2 xAxis, v2 yAxis, 
                    pixel_buffer_32 *normalMap, environment_map *envMaps);
internal void
PushRect(render_group *renderGroup, v3 p, v3 dim, v3 color, v2 xAxis, v2 yAxis);

#endif
