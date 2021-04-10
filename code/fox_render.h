#ifndef FOX_RENDER_H
#define FOX_RENDER_H


struct loaded_bitmap
{
    i32 width;
    i32 height;
    i32 pitch;

    void *data;
};

struct render_camera
{
    world_p worldP;
    v3 p;
    v3 zFromBasisPlane; // TODO : Correct way to implement render camera z?
    v3 halfDim;
};

enum render_entry_type
{
    render_entry_type_rect,
    render_entry_type_bitmap,
};

struct render_entry_rect
{
    render_entry_type entryType;

    v2 originP;
    v2 xAxis;
    v2 yAxis;

    v4 color;
};

struct render_entry_bitmap
{
    render_entry_type entryType;

    v2 originP;
    v2 xAxis;
    v2 yAxis;

    loaded_bitmap *bitmap; // TODO : Might be a better idea to store as non-pointer value?
    v4 color;
};
 
struct render_group
{
    r32 mToPixel;
    render_camera camera;

    temporary_memory renderMemory;
};
#endif