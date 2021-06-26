#include "monsoon_render.h"

internal void
ClearPixelBuffer(pixel_buffer_32 *buffer, r32 R, r32 G, r32 B, r32 A = 1)
{
    u32 color = (u32)((RoundR32ToInt32(A * 255.0f) << 24) |
                    (RoundR32ToInt32(R * 255.0f) << 16) |
                    (RoundR32ToInt32(G * 255.0f) << 8) |
                    (RoundR32ToInt32(B * 255.0f) << 0));

    // TODO : What should we take care of when we properly clear the buffer?
    u8 *row = (u8 *)buffer->memory;
    for(i32 Y = 0;
        Y < buffer->height;
        ++Y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 X = 0;
            X < buffer->width;
            ++X)
        {
            *pixel++ = color;
        }

        row += buffer->pitch;
    }
}

// NOTE : MacOS offscreen buffer is bottom-up 
internal void
DrawRectangle(pixel_buffer_32 *buffer, v2 p, v2 xAxis, v2 yAxis,
            r32 R, r32 G, r32 B, r32 A = 1)
{    
    v2 p0 = p;
    v2 p1 = p + xAxis;
    v2 p2 = p + yAxis;
    v2 p3 = p + xAxis + yAxis;

    i32 minX = RoundR32ToInt32(Minimum(Minimum(p0.x, p1.x), Minimum(p2.x, p3.x)));
    i32 minY = RoundR32ToInt32(Minimum(Minimum(p0.y, p1.y), Minimum(p2.y, p3.y)));
    i32 maxX = RoundR32ToInt32(Maximum(Maximum(p0.x, p1.x), Maximum(p2.x, p3.x)));
    i32 maxY = RoundR32ToInt32(Maximum(Maximum(p0.y, p1.y), Maximum(p2.y, p3.y)));

    if(minX < 0)
    {
        minX = 0;
    }
    if(maxX > buffer->width)
    {
        maxX = buffer->width;
    }
    if(minY < 0)
    {
        minY = 0;
    }
    if(maxY > buffer->height)
    {
        maxY = buffer->height;
    }   

    // NOTE : Bit pattern for the pixel : AARRGGBB
    //
    u32 color = (u32)((RoundR32ToInt32(A * 255.0f) << 24) |
                    (RoundR32ToInt32(R * 255.0f) << 16) |
                    (RoundR32ToInt32(G * 255.0f) << 8) |
                    (RoundR32ToInt32(B * 255.0f) << 0));

    u8 *row = (u8 *)buffer->memory + 
                    buffer->pitch*minY + 
                    buffer->bytesPerPixel*minX;
    for(i32 y = minY;
        y < maxY;
        ++y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = minX;
            x < maxX;
            ++x)
        {
            v2 v = V2(x, y) - p;
            r32 vDotXAxis = Dot(v, xAxis);
            r32 vDotYAxis = Dot(v, yAxis);
            r32 LengthSquareOfXAxis = LengthSquare(xAxis);
            r32 LengthSquareOfYAxis = LengthSquare(yAxis);

            if(!(vDotXAxis < 0 ||
                vDotYAxis < 0 ||
                vDotXAxis > LengthSquareOfXAxis||
                vDotYAxis > LengthSquareOfYAxis))
            {
                *pixel++ = color;
            }
        }
        row += buffer->pitch;
    }
}

// NOTE : p is ALWAYS the pixel p based on the entity p.
// only the alignment can be used to adjust the bmp drawing position.
internal void
DrawBMP(pixel_buffer_32 *Buffer, pixel_buffer_32 *pixels,
        v2 p, v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1), v2 alignment = V2(0, 0))
{
    Assert(pixels);

    // TODO : Do we even need this alignment value?
    // because we always starts at p - halfDim to p + halfdim, and
    // the bmp will just fill up the space that we requested.
    // If it turns out that we need this alignment value, we have to adjust
    // the value based on the dim?
    //p += alignment;
    v2 p0 = p;
    v2 p1 = p + xAxis;
    v2 p2 = p + yAxis;
    v2 p3 = p + xAxis + yAxis;

    i32 minX = RoundR32ToInt32(Minimum(Minimum(p0.x, p1.x), Minimum(p2.x, p3.x)));
    i32 minY = RoundR32ToInt32(Minimum(Minimum(p0.y, p1.y), Minimum(p2.y, p3.y)));
    i32 maxX = RoundR32ToInt32(Maximum(Maximum(p0.x, p1.x), Maximum(p2.x, p3.x)));
    i32 maxY = RoundR32ToInt32(Maximum(Maximum(p0.y, p1.y), Maximum(p2.y, p3.y)));

    i32 SourceOffsetX = 0;
    i32 SourceOffsetY = 0;
    
    if(minX < 0) 
    {
        SourceOffsetX = -minX;
        minX = 0;
    }
    if(maxX > Buffer->width)
    {
        maxX = Buffer->width;
    }
    if(minY < 0)
    {
        SourceOffsetY = -minY;
        minY = 0;
    }
    if(maxY > Buffer->height)
    {
        maxY = Buffer->height;
    }   

#if 0
    // NOTE : To see the region of the whole BMP, enable this.
    DrawRectangle(Buffer, V2(minX, minY), dim, 1, 0, 0);
#endif

    u8 *row = (u8 *)Buffer->memory + 
                    Buffer->pitch*minY + 
                    Buffer->bytesPerPixel*minX;
    for(i32 y = minY;
        y < maxY;
        ++y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = minX;
            x < maxX;
            ++x)
        {
            // TODO : if the p has fractional value, it might make the dot value
            // to be sligtly off(i.e -0.33333f), and the pixel can fail the test below.
            // What will be a proper way to handle this?(Round?)
            v2 v = V2(x, y) - p;
            r32 vDotXAxis = Dot(v, xAxis);
            r32 vDotYAxis = Dot(v, yAxis);
            r32 LengthSquareOfXAxis = LengthSquare(xAxis);
            r32 LengthSquareOfYAxis = LengthSquare(yAxis);

            if(!(vDotXAxis < 0 ||
                vDotYAxis < 0 ||
                vDotXAxis > LengthSquareOfXAxis||
                vDotYAxis > LengthSquareOfYAxis))
            {
                
                v2i textureCoord;
                textureCoord.x = (vDotXAxis/LengthSquareOfXAxis)*pixels->width;
                textureCoord.y = (vDotYAxis/LengthSquareOfYAxis)*pixels->height;

                u32 *Sourcepixel = (u32 *)pixels->memory + 
                                    textureCoord.y*pixels->width +
                                    textureCoord.x;

                u32 DestA = ((*pixel >> 24) & 0x000000ff);
                r32 R32DestA = DestA/255.0f;
                u32 DestR = (*pixel >> 16) & 0x000000ff;
                u32 DestG = (*pixel >> 8) & 0x000000ff;
                u32 DestB = (*pixel >> 0) & 0x000000ff;

                u32 SourceA = ((*Sourcepixel >> 24) & 0x000000ff);
                r32 R32SourceA = SourceA/255.0f;
                u32 SourceR = (*Sourcepixel >> 16) & 0x000000ff;
                u32 SourceG = (*Sourcepixel >> 8) & 0x000000ff;
                u32 SourceB = (*Sourcepixel >> 0) & 0x000000ff;

                r32 InvSourceA = 1.0f - R32SourceA;
                r32 ResultA = DestA*InvSourceA + SourceA; 
                // NOTE : Source R, G, B are pre multiplied by the SourceA
                // TODO : Overly Overlapped pixelss make wrong color value, might wanna do something with that?
                r32 ResultR = DestR*InvSourceA + SourceR;
                r32 ResultG = DestG*InvSourceA + SourceG;
                r32 ResultB = DestB*InvSourceA + SourceB;

                u32 ResultColor = (RoundR32ToUInt32(ResultA) << 24) |
                                    ((RoundR32ToUInt32(ResultR) & 0x000000ff) << 16) |
                                    ((RoundR32ToUInt32(ResultG) & 0x000000ff) << 8) |
                                    ((RoundR32ToUInt32(ResultB) & 0x000000ff) << 0);

                *pixel = ResultColor;

            }
#if 0
            else
            {
                *pixel++ = 0xffff0000;
            }
#endif
            pixel++;
        }


        row += Buffer->pitch;
    }
}

internal void
PushRect(render_group *RenderGroup, v3 p, v3 dim, v3 color, v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
{
    u8 *Memory = PushSize(&RenderGroup->renderMemory, sizeof(render_element_header) + sizeof(render_element_rect));
    render_element_header *header = (render_element_header *)Memory;
    render_element_rect *element = (render_element_rect *)(Memory + sizeof(render_element_header));

    v2 pixelDim = RenderGroup->metersToPixels*dim.xy;
    v2 pixelP = RenderGroup->bufferHalfDim + RenderGroup->metersToPixels*p.xy - 0.5f*pixelDim;

    header->Type = RenderElementType_Rect;
    header->xAxis = pixelDim.x*xAxis;
    header->yAxis = pixelDim.y*yAxis;
    v2 AdjustmentToMakeBMPCentered = 0.5f*pixelDim - (0.5f*header->xAxis + 0.5f*header->yAxis);
    header->p = pixelP + AdjustmentToMakeBMPCentered;

    element->color = color;

    RenderGroup->elementCount++;
}

// TODO : Allow xAxis and yAxis not to be perpendicular?
internal void
PushBMP(render_group *RenderGroup, pixel_buffer_32 *pixels, v3 p, v3 dim, v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
{
    u8 *Memory = PushSize(&RenderGroup->renderMemory, sizeof(render_element_header) + sizeof(render_element_bmp));
    render_element_header *header = (render_element_header *)Memory;
    render_element_bmp *element = (render_element_bmp *)(Memory + sizeof(render_element_header));

    v2 pixelDim = RenderGroup->metersToPixels*dim.xy;
    v2 pixelP = RenderGroup->bufferHalfDim + RenderGroup->metersToPixels*p.xy - 0.5f*pixelDim;

    header->Type = RenderElementType_BMP;
    header->xAxis = pixelDim.x*xAxis;
    header->yAxis = pixelDim.y*yAxis;
    v2 AdjustmentToMakeBMPCentered = 0.5f*pixelDim - (0.5f*header->xAxis + 0.5f*header->yAxis);
    header->p = pixelP + AdjustmentToMakeBMPCentered;

    element->pixels = *pixels;
    element->alignment.x = (((r32)pixels->alignment.x/(r32)pixels->width)*pixelDim.x);
    element->alignment.y = (((r32)pixels->alignment.y/(r32)pixels->height)*pixelDim.y);

    RenderGroup->elementCount++;
}

// TODO : Better buffer clearing function
internal void 
ClearBuffer(game_offscreen_buffer *Buffer, v3 color)
{
    u32 C = (u32)((RoundR32ToUInt32(color.x*255.0f) << 16) | 
                (RoundR32ToUInt32(color.y*255.0f) << 8) | 
                (RoundR32ToUInt32(color.z*255.0f) << 0));

    u8 *FirstpixelOfrow = (u8 *)Buffer->memory;
    for(int row = 0;
        row < Buffer->height;
        ++row)
    {
        u32 *pixel = (u32 *)(FirstpixelOfrow);
        for(int Column = 0;
            Column < Buffer->width;
            ++Column)
        {
            *pixel++ = C;
        }

        FirstpixelOfrow += Buffer->pitch;
    }
}

internal void
RenderRenderGroup(render_group *RenderGroup, pixel_buffer_32 *pixels)
{
    u8 *Base = RenderGroup->renderMemory.base;
    for(u32 ElementIndex = 0;
        ElementIndex < RenderGroup->elementCount;
        ++ElementIndex)
    {
        render_element_header *header = (render_element_header *)Base;
        Base += sizeof(*header);
        switch(header->Type)
        {
#if 1
            case RenderElementType_Rect :
            {
                render_element_rect *element = (render_element_rect *)Base;
                DrawRectangle(pixels, header->p, header->xAxis, header->yAxis,
                                element->color.x, element->color.y, element->color.z);
                Base += sizeof(*element);
            }break;
#endif

            case RenderElementType_BMP :
            {
                render_element_bmp *element = (render_element_bmp *)Base;
                DrawBMP(pixels, &element->pixels, 
                        // TODO : Bitmap resize based on the entity dim!!
                        header->p, header->xAxis, header->yAxis,
                        element->alignment);
                Base += sizeof(*element);
            }break;
        }
    }
}

#include <string.h>
internal void
DrawOffScreenBuffer(game_offscreen_buffer *OffscreenBuffer, pixel_buffer_32 *FinalpixelBuffer)
{
    Assert(OffscreenBuffer->width == FinalpixelBuffer->width &&
            OffscreenBuffer->height == FinalpixelBuffer->height &&
            OffscreenBuffer->bytesPerPixel == FinalpixelBuffer->bytesPerPixel)

    memcpy(OffscreenBuffer->memory, FinalpixelBuffer->memory, FinalpixelBuffer->pitch*FinalpixelBuffer->height);
}

internal pixel_buffer_32
MakeEmptyPixelBuffer32(u32 width, u32 height, v2 alignment = V2(0, 0))
{
    pixel_buffer_32 buffer = {};

    buffer.width = width;
    buffer.height = height;
    buffer.bytesPerPixel = 4;
    buffer.pitch = buffer.bytesPerPixel * buffer.width;

    return buffer;
}

