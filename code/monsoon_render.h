#ifndef MONSOON_RENDER_H
#define MONSOON_RENDER_H

enum render_element_type
{
    RenderElementType_Rect,
    RenderElementType_BMP,
};

struct render_element_header
{
    render_element_type Type;
    // TODO : This should be v3 so can we can sort using Z
    v2 P; 
};

struct render_element_rect
{
    v2 Dim;
    v3 Color;
};

struct render_element_bmp
{
    // TODO : Do not store the whole BMP info, fix this when the game has compressed asset
    // Maybe store a pointer to the memory_loaded asset?
    debug_loaded_bmp LoadedBMP;
    v2 Dim;
};

struct render_group
{
    memory_arena Arena;

    u32 ElementCount;
    r32 MetersToPixels;

    v2 BufferHalfDim;
};

#endif
