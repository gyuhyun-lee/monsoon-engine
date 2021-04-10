#include "fox_render.h"

internal v4
AwesomeColor(r32 a = 255.0f)
{
    v4 result = V4(RandomBetween(64, 255), 
                RandomBetween(64, 255), 
                RandomBetween(64, 255), a);

    return result;
}

internal void
ClearBuffer(game_screen_buffer *buffer, u32 color)
{
    // NOTE : Bottom left corner of the screen buffer is 0, 0
    u8 *row = (u8 *)buffer->memory;
    for(i32 y = 0;
            y < buffer->height;
            ++y)
    {
        u32 *pixel = (u32 *)row;

        for(i32 x = 0;
                x < buffer->width;
                ++x)
        {
#if 0 
            if((x >= buffer->width/2 - 1 && x <= buffer->width/2 + 1) ||
                (y >= buffer->height/2 - 1 && y <= buffer->height/2 +1))
            {
                *pixel++ = 0xff00ffff;
            }
            else
#endif
            {
                *pixel++ = color;
            }
        }
        
        row += buffer->pitch;
    }
}

inline v4
GetARGB(u32 pixel)
{
    v4 result = {};

    result.a = ((pixel & 0xff000000) >> 24)/255.0f;
    result.r = ((pixel & 0x00ff0000) >> 16)/255.0f;
    result.g = ((pixel & 0x0000ff00) >> 8)/255.0f;
    result.b = ((pixel & 0x000000ff) >> 0)/255.0f;

    return result;
}

internal b32
TestTwoSegCollide(v2 pixel, v2 checkRectCenterP, v2 segStart, v2 segEnd)
{
    b32 result = false;

    v2 rayStart = pixel;
    v2 rayDir = checkRectCenterP - pixel;
    v2 segDir = segEnd - segStart;

    r32 t2 = (rayDir.x*(segStart.y - rayStart.y) + rayDir.y*(rayStart.x - segStart.x))/
            (segDir.x*rayDir.y - segDir.y*rayDir.x);

    r32 t1 = (segStart.x+segDir.x*t2-rayStart.x)/rayDir.x;
    // r32 t1 = (segStart.y+segDir.y*t2-rayStart.y)/rayDir.y;

    if(t1 >= 0.0f && t1 < 1.0f &&
        t2 >= 0.0f && t2 < 1.0f)
    {
        result = true;
    }

    return result;
}

// NOTE : This function assumes that all parameters are relative to seg1Start
// which means seg1Start is 0
internal b32
RelTestTwoSegCollide(v2 seg1End, v2 seg2Start, v2 seg2End)
{
    b32 result = false;

    v2 seg2Dir = seg2End - seg2Start;

    r32 t2 = (seg1End.x*(seg2Start.y) + seg1End.y*(-seg2Start.x))/
            (seg2Dir.x*seg1End.y - seg2Dir.y*seg1End.x);

    r32 t1 = (seg2Start.x+seg2Dir.x*t2)/seg1End.x;
    // r32 t1 = (segStart.y+segDir.y*t2-rayStart.y)/rayDir.y;

    if(t1 >= 0.0f && t1 < 1.0f &&
        t2 >= 0.0f && t2 < 1.0f)
    {
        result = true;
    }

    return result;
}

internal void
DrawRectangleSlowly(game_screen_buffer *buffer,
                    v2 originP, v2 xAxis, v2 yAxis,
                        v4 color)
{
    v2 pos00 = originP;
    v2 pos10 = originP + xAxis;
    v2 pos01 = originP + yAxis;
    v2 pos11 = originP + xAxis + yAxis;

    i32 checkRectMinX = Maximum(Round(Minimum(Minimum(pos00.x, pos11.x), Minimum(pos01.x, pos10.x))), 0);
    i32 checkRectMinY = Maximum(Round(Minimum(Minimum(pos00.y, pos11.y), Minimum(pos01.y, pos10.y))), 0);
    i32 checkRectMaxX = Minimum(Round(Maximum(Maximum(pos00.x, pos11.x), Maximum(pos01.x, pos10.x))), buffer->width);
    i32 checkRectMaxY = Minimum(Round(Maximum(Maximum(pos00.y, pos11.y), Maximum(pos01.y, pos10.y))), buffer->height);

    v2 checkRectCenterP = (pos00 + pos11)/2.0f;

    i32 width = checkRectMaxX - checkRectMinX;
    i32 height = checkRectMaxY - checkRectMinY;

#if 0
    {
    u8 *row = (u8 *)buffer->memory + checkRectMinY*buffer->pitch + checkRectMinX*Bytes_Per_Pixel;
    for(r32 y = 0;
            y < (r32)height;
            y += 1.0f)
    {
        u32 *pixel = (u32 *)row;

        for(r32 x = 0;
                x < (r32)width;
                x += 1.0f)
        {
            {
                // These all range from 0 to 1
                v4 bufferColor = GetARGB(*pixel);

                r32 sa = color.a/255.0f;
                r32 sr = color.r/255.0f;
                r32 sg = color.g/255.0f;
                r32 sb = color.b/255.0f;

                r32 da = 255.0f*Lerp(bufferColor.a, sa, sa);
                r32 dr = 255.0f*Lerp(bufferColor.r, sa, sr);
                r32 dg = 255.0f*Lerp(bufferColor.g, sa, sg);
                r32 db = 255.0f*Lerp(bufferColor.b, sa, sb);

                // NOTE : Put it back as a, r, g, b order
                u32 resultColor = (((u32)(da + 0.5f) << 24) |
                                    ((u32)(dr + 0.5f) << 16) |
                                    ((u32)(dg + 0.5f) << 8) |
                                    ((u32)(db + 0.5f) << 0));   

                *pixel++ = 0xffff0000;
            }
        }
        
        row += buffer->pitch;
    }
    }
#endif

    v2 relP00 = pos00 - checkRectCenterP;
    v2 relP01 = pos01 - checkRectCenterP;
    v2 relP10 = pos10 - checkRectCenterP;
    v2 relP11 = pos11 - checkRectCenterP;

    u8 *row = (u8 *)buffer->memory + checkRectMinY*buffer->pitch + checkRectMinX*Bytes_Per_Pixel;
    for(r32 y = 0;
            y < (r32)height;
            y += 1.0f)
    {
        u32 *pixel = (u32 *)row;

        for(r32 x = 0;
                x < (r32)width;
                x += 1.0f)
        {
            v2 p = V2(x, y) + V2((r32)checkRectMinX, (r32)checkRectMinY);

            r32 adMinusbc = xAxis.x*yAxis.y - xAxis.y*yAxis.x;
            v2 originRelP = p - originP;
            r32 t1 = (yAxis.x*originRelP.y - yAxis.y*originRelP.x)/-adMinusbc;
            r32 t2 = (xAxis.x*originRelP.y - xAxis.y*originRelP.x)/adMinusbc;

            if(t1 >= 0.0f && t1 < 1.0f &&
                t2 >= 0.0f && t2 < 1.0f)
            {
                // These all range from 0 to 1
                v4 bufferColor = GetARGB(*pixel);

                r32 sa = color.a/255.0f;
                r32 sr = color.r/255.0f;
                r32 sg = color.g/255.0f;
                r32 sb = color.b/255.0f;

                r32 da = 255.0f*Lerp(bufferColor.a, sa, sa);
                r32 dr = 255.0f*Lerp(bufferColor.r, sa, sr);
                r32 dg = 255.0f*Lerp(bufferColor.g, sa, sg);
                r32 db = 255.0f*Lerp(bufferColor.b, sa, sb);

                // NOTE : Put it back as a, r, g, b order
                u32 resultColor = (((u32)(da + 0.5f) << 24) |
                                    ((u32)(dr + 0.5f) << 16) |
                                    ((u32)(dg + 0.5f) << 8) |
                                    ((u32)(db + 0.5f) << 0));   

                *pixel = resultColor;

            }
            
            ++pixel;
        }
        
        row += buffer->pitch;
    }
}

internal void
DrawRectangle(game_screen_buffer *buffer,
            v2 minCorner, v2 maxCorner,
            v4 color)
{
    i32 fillRectMinX = Round(minCorner.x);
    i32 fillRectMinY = Round(minCorner.y);
    i32 fillRectMaxX = Round(maxCorner.x);
    i32 fillRectMaxY = Round(maxCorner.y);

    fillRectMaxX = Minimum(fillRectMaxX, buffer->width);
    fillRectMaxY = Minimum(fillRectMaxY, buffer->height);
    // If minCorner gets bigger than the gameBuffer. the for loop will not draw anything
    // so I don't have to worry about buffer overflow in that case.
    fillRectMinX = Maximum(fillRectMinX, 0); 
    fillRectMinY = Maximum(fillRectMinY, 0);

    u8 *row = (u8 *)buffer->memory + fillRectMinY*buffer->pitch + fillRectMinX*Bytes_Per_Pixel;
    for(i32 y = fillRectMinY;
            y < fillRectMaxY;
            ++y)
    {
        u32 *pixel = (u32 *)row;

        for(i32 x = fillRectMinX;
                x < fillRectMaxX;
                ++x)
        {
            // These all range from 0 to 1
            v4 bufferColor = GetARGB(*pixel);

        	r32 sa = color.a/255.0f;
        	r32 sr = color.r/255.0f;
        	r32 sg = color.g/255.0f;
        	r32 sb = color.b/255.0f;

        	r32 da = 255.0f*Lerp(bufferColor.a, sa, sa);
        	r32 dr = 255.0f*Lerp(bufferColor.r, sa, sr);
        	r32 dg = 255.0f*Lerp(bufferColor.g, sa, sg);
        	r32 db = 255.0f*Lerp(bufferColor.b, sa, sb);

            // NOTE : Put it back as a, r, g, b order
            u32 resultColor = (((u32)(da + 0.5f) << 24) |
			                    ((u32)(dr + 0.5f) << 16) |
			                    ((u32)(dg + 0.5f) << 8) |
			                    ((u32)(db + 0.5f) << 0));	

            *pixel++ = resultColor;
        }
        
        row += buffer->pitch;
    }
}

internal void
DrawBitmapSlowly(game_screen_buffer *buffer,
            loaded_bitmap *bitmap,
            v2 originP,
            v2 xAxis, v2 yAxis,
            v4 color)
{    
    v2 pos00 = originP;
    v2 pos10 = originP + xAxis;
    v2 pos01 = originP + yAxis;
    v2 pos11 = originP + xAxis + yAxis;

    i32 checkRectMinX = Maximum(Round(Minimum(Minimum(pos00.x, pos11.x), Minimum(pos01.x, pos10.x))), 0);
    i32 checkRectMinY = Maximum(Round(Minimum(Minimum(pos00.y, pos11.y), Minimum(pos01.y, pos10.y))), 0);
    i32 checkRectMaxX = Minimum(Round(Maximum(Maximum(pos00.x, pos11.x), Maximum(pos01.x, pos10.x))), buffer->width);
    i32 checkRectMaxY = Minimum(Round(Maximum(Maximum(pos00.y, pos11.y), Maximum(pos01.y, pos10.y))), buffer->height);

    v2 checkRectCenterP = (pos00 + pos11)/2.0f;
    i32 width = checkRectMaxX - checkRectMinX;
    i32 height = checkRectMaxY - checkRectMinY;

#if 0
    {
    u8 *row = (u8 *)buffer->memory + checkRectMinY*buffer->pitch + checkRectMinX*Bytes_Per_Pixel;
    for(r32 y = 0;
            y < (r32)height;
            y += 1.0f)
    {
        u32 *pixel = (u32 *)row;

        for(r32 x = 0;
                x < (r32)width;
                x += 1.0f)
        {
            {
                // These all range from 0 to 1
                v4 bufferColor = GetARGB(*pixel);

                r32 sa = color.a/255.0f;
                r32 sr = color.r/255.0f;
                r32 sg = color.g/255.0f;
                r32 sb = color.b/255.0f;

                r32 da = 255.0f*Lerp(bufferColor.a, sa, sa);
                r32 dr = 255.0f*Lerp(bufferColor.r, sa, sr);
                r32 dg = 255.0f*Lerp(bufferColor.g, sa, sg);
                r32 db = 255.0f*Lerp(bufferColor.b, sa, sb);

                // NOTE : Put it back as a, r, g, b order
                u32 resultColor = (((u32)(da + 0.5f) << 24) |
                                    ((u32)(dr + 0.5f) << 16) |
                                    ((u32)(dg + 0.5f) << 8) |
                                    ((u32)(db + 0.5f) << 0));   

                *pixel++ = 0xffff00ff;
            }
        }
        
        row += buffer->pitch;
    }
    }
#endif

    v2 relP00 = pos00 - checkRectCenterP;
    v2 relP01 = pos01 - checkRectCenterP;
    v2 relP10 = pos10 - checkRectCenterP;
    v2 relP11 = pos11 - checkRectCenterP;

    r32 xAxisLength = Length(xAxis);
    r32 yAxisLength = Length(yAxis);
    r32 bitmapXPerPixel = bitmap->width/xAxisLength;
    r32 bitmapYPerPixel = bitmap->height/yAxisLength;

    v4 color01 = color/255.0f;

    u8 *row = (u8 *)buffer->memory + checkRectMinY*buffer->pitch + checkRectMinX*Bytes_Per_Pixel;
    for(r32 y = 0.0f;
            y < height;
            y += 1.0f)
    {
        u32 *pixel = (u32 *)row;

        for(r32 x = 0.0f;
                x < width;
                x += 1.0f)
        {
            v2 p = V2(x, y) + V2((r32)checkRectMinX, (r32)checkRectMinY);

            r32 adMinusbc = xAxis.x*yAxis.y - xAxis.y*yAxis.x;
            v2 originRelP = p - originP;
            r32 t1 = (yAxis.x*originRelP.y - yAxis.y*originRelP.x)/-adMinusbc;
            r32 t2 = (xAxis.x*originRelP.y - xAxis.y*originRelP.x)/adMinusbc;

            if(t1 >= 0.0f && t1 < 1.0f &&
                t2 >= 0.0f && t2 < 1.0f)
            {
                u32 bitmapX = (u32)(t1*bitmap->width);
                u32 bitmapY = (u32)(t2*bitmap->height);

                // These all range from 0 to 1
                v4 bufferColor01 = GetARGB(*pixel);

                u32 *source = (u32 *)bitmap->data + bitmapY*bitmap->width + bitmapX;
                v4 sourceColor01 = GetARGB(*source);

                r32 blendeda = 1.0f; // TODO : How to blend the bitmap with input color correctly?
                r32 blendedr = Lerp(color01.r, blendeda, sourceColor01.r);
                r32 blendedg = Lerp(color01.g, blendeda, sourceColor01.g);
                r32 blendedb = Lerp(color01.b, blendeda, sourceColor01.b);

                r32 da = 255.0f*Lerp(bufferColor01.a, sourceColor01.a, blendeda);
                r32 dr = 255.0f*Lerp(bufferColor01.r, sourceColor01.a, blendedr);
                r32 dg = 255.0f*Lerp(bufferColor01.g, sourceColor01.a, blendedg);
                r32 db = 255.0f*Lerp(bufferColor01.b, sourceColor01.a, blendedb);

                // NOTE : Put it back as a, r, g, b order
                u32 resultColor = (((u32)(da + 0.5f) << 24) |
                                    ((u32)(dr + 0.5f) << 16) |
                                    ((u32)(dg + 0.5f) << 8) |
                                    ((u32)(db + 0.5f) << 0));   
                *pixel = resultColor;
            }

            ++pixel;
        }
        
        row += buffer->pitch;
    }
}

internal void
DrawBitmap(game_screen_buffer *buffer,
			loaded_bitmap *bitmap,
            v2 minCorner, v2 maxCorner,
            v4 color)
{    
	i32 fillRectMinX = Round(minCorner.x);
    i32 fillRectMinY = Round(minCorner.y);
    i32 fillRectMaxX = Round(maxCorner.x);
    i32 fillRectMaxY = Round(maxCorner.y);

    fillRectMaxX = Minimum(fillRectMaxX, buffer->width);
    fillRectMaxY = Minimum(fillRectMaxY, buffer->height);
    // If minCorner gets bigger than the gameBuffer. the for loop will not draw anything
    // so I don't have to worry about buffer overflow in that case.
    fillRectMinX = Maximum(fillRectMinX, 0); 
    fillRectMinY = Maximum(fillRectMinY, 0);

    i32 width = fillRectMaxX - fillRectMinX;
    i32 height = fillRectMaxY - fillRectMinY;

    v4 color01 = color/255.0f;

    u8 *row = (u8 *)buffer->memory + fillRectMinY*buffer->pitch + fillRectMinX*Bytes_Per_Pixel;
    for(r32 y = 0.0f;
            y < height;
            y += 1.0f)
    {
        u32 *pixel = (u32 *)row;
        r32 axisY = y/(r32)height;
        u32 pixelY = (u32)(bitmap->height * axisY)*bitmap->width;

        for(r32 x = 0.0f;
                x < width;
                x += 1.0f)
        {
            r32 axisX = x/(r32)width;
            u32 pixelX = (u32)(bitmap->width * axisX);

            // These all range from 0 to 1
            v4 bufferColor = GetARGB(*pixel);

            u32 *source = (u32 *)bitmap->data + (u32)(pixelY) + pixelX;
            v4 sourceColor01 = GetARGB(*source);

            r32 blendeda = 1.0f; // TODO : How to blend the bitmap with input color correctly?
            r32 blendedr = Lerp(color01.r, blendeda, sourceColor01.r);
            r32 blendedg = Lerp(color01.g, blendeda, sourceColor01.g);
            r32 blendedb = Lerp(color01.b, blendeda, sourceColor01.b);

            r32 da = 255.0f*Lerp(bufferColor.a, sourceColor01.a, blendeda);
            r32 dr = 255.0f*Lerp(bufferColor.r, sourceColor01.a, blendedr);
            r32 dg = 255.0f*Lerp(bufferColor.g, sourceColor01.a, blendedg);
            r32 db = 255.0f*Lerp(bufferColor.b, sourceColor01.a, blendedb);

            // NOTE : Put it back as a, r, g, b order
            u32 resultColor = (((u32)(da + 0.5f) << 24) |
                                ((u32)(dr + 0.5f) << 16) |
                                ((u32)(dg + 0.5f) << 8) |
                                ((u32)(db + 0.5f) << 0));   

            *pixel++ = resultColor;
        }
        
        row += buffer->pitch;
    }
}

struct buffer_geo
{
    v2 originP;

    v2 xAxis;
    v2 yAxis;
};

internal buffer_geo
GetBufferGeoWithCenteredP(game_screen_buffer *buffer, render_group *renderGroup, 
                v3 p, v3 halfDim, v2 xAxis, v2 yAxis)
{
    buffer_geo result = {};

    v3 pixelP = (p - renderGroup->camera.p) / renderGroup->mToPixel;
    pixelP += V3(buffer->center, 0.0f);
    v3 halfDimInPixel = halfDim / renderGroup->mToPixel;

    result.xAxis = 2.0f*halfDimInPixel.x*xAxis;
    result.yAxis = 2.0f*halfDimInPixel.y*yAxis;
    result.originP = pixelP.xy - 0.5f*(result.xAxis + result.yAxis);

    return result;
}

internal void
PushRect(game_screen_buffer *buffer, render_group *renderGroup,
                        v3 p, v3 halfDim, 
                        v4 color = V4(255, 255, 255, 255),
                        v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
                        
{
    render_entry_rect *entry = (render_entry_rect *)AllocateTemporaryMemory(&renderGroup->renderMemory, sizeof(render_entry_rect));
    entry->entryType = render_entry_type_rect;

    buffer_geo bufferGeo = GetBufferGeoWithCenteredP(buffer, renderGroup, p, halfDim, xAxis, yAxis);

    entry->xAxis = bufferGeo.xAxis;
    entry->yAxis = bufferGeo.yAxis;
    entry->originP = bufferGeo.originP;

    entry->color = color;
}

internal void
PushRectWithAngle(game_screen_buffer *buffer, render_group *renderGroup,
                        v3 p, v3 halfDim, 
                        v4 color = V4(255, 255, 255, 255),
                        r32 angle = 0.0f)
                        
{
    render_entry_rect *entry = (render_entry_rect *)AllocateTemporaryMemory(&renderGroup->renderMemory, sizeof(render_entry_rect));
    entry->entryType = render_entry_type_rect;

    v2 xAxis = V2(Cos(angle), Sin(angle));
    v2 yAxis = V2(Cos(angle + Rad(90)), Sin(angle + Rad(90)));
    buffer_geo bufferGeo = GetBufferGeoWithCenteredP(buffer, renderGroup, p, halfDim, xAxis, yAxis);

    entry->xAxis = bufferGeo.xAxis;
    entry->yAxis = bufferGeo.yAxis;
    entry->originP = bufferGeo.originP;

    entry->color = color;
}

internal void
PushRectOutline(game_screen_buffer *buffer, render_group *renderGroup,
                        v3 p, v3 halfDim, 
                        v4 color = V4(255, 255, 255, 255),
                        v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
{
    r32 thickness = 3.0f;
    r32 halfThickness = thickness / 2.0f;
    v3 pOffsetForHorizontalWall = (halfDim.x - halfThickness)*V3(xAxis, 0.0f);
    v3 pOffsetForVerticalWall = (halfDim.x - halfThickness)*V3(yAxis, 0.0f);

    v3 pRight = p + pOffsetForHorizontalWall;
    v3 pUp = p + pOffsetForVerticalWall;
    v3 pLeft = p - pOffsetForHorizontalWall;
    v3 pDown = p - pOffsetForVerticalWall;

    PushRect(buffer, renderGroup, pRight, V3(halfThickness, halfDim.y, 0.0f), color, xAxis, yAxis);
    PushRect(buffer, renderGroup, pUp, V3(halfDim.x, halfThickness, 0.0f), color, xAxis, yAxis);
    PushRect(buffer, renderGroup, pLeft, V3(halfThickness, halfDim.y, 0.0f), color, xAxis, yAxis);
    PushRect(buffer, renderGroup, pDown, V3(halfDim.x, halfThickness, 0.0f), color, xAxis, yAxis);
}

internal void
PushBitmap(game_screen_buffer *buffer, render_group *renderGroup, 
                    loaded_bitmap *bitmap,
                    v3 p, v3 halfDim, 
                    v4 color = V4(255, 255, 255, 255),
                    v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
{
    render_entry_bitmap *entry = (render_entry_bitmap *)AllocateTemporaryMemory(&renderGroup->renderMemory, sizeof(render_entry_bitmap));
    entry->entryType = render_entry_type_bitmap;


    buffer_geo bufferGeo = GetBufferGeoWithCenteredP(buffer, renderGroup, p, halfDim, xAxis, yAxis);

    entry->xAxis = bufferGeo.xAxis;
    entry->yAxis = bufferGeo.yAxis;
    entry->originP = bufferGeo.originP;

    entry->bitmap = bitmap;
    entry->color = color;
}

internal void
RenderAllEntries(render_group *renderGroup, game_screen_buffer *buffer)
{
    u64 currentRenderMemoryIndex = 0;
    temporary_memory *renderMemory = &renderGroup->renderMemory;

    while(currentRenderMemoryIndex < renderMemory->used)
    {
        render_entry_type *type = (render_entry_type *)(renderMemory->base + currentRenderMemoryIndex);
        switch(*type)
        {
            case render_entry_type_rect:
            {
                render_entry_rect *entry = (render_entry_rect *)type;

                DrawRectangleSlowly(buffer, 
                                    entry->originP, entry->xAxis, entry->yAxis,
                                    entry->color);

                currentRenderMemoryIndex += sizeof(render_entry_rect);
            }break;

            case render_entry_type_bitmap:
            {
                render_entry_bitmap *entry = (render_entry_bitmap *)type;
                DrawBitmapSlowly(buffer, 
                            entry->bitmap,
                            entry->originP, entry->xAxis, entry->yAxis,
                            entry->color);

                currentRenderMemoryIndex += sizeof(render_entry_bitmap);
            }break;
        }
    }
}

internal void
DrawRectangleRelToCamera(game_screen_buffer *buffer, render_group *renderGroup, 
                        v3 p, v3 halfDim, v4 color)
{
    v3 pixelP = (p - renderGroup->camera.p) / renderGroup->mToPixel;
    pixelP += V3(buffer->center, 0.0f);
    v3 halfDimInPixel = halfDim / renderGroup->mToPixel;

    v2 minCorner = pixelP.xy - halfDimInPixel.xy;
    v2 maxCorner = pixelP.xy + halfDimInPixel.xy;

    DrawRectangle(buffer, minCorner, maxCorner, color);
}

internal void
DrawBitmapRelToCamera(game_screen_buffer *buffer, render_group *renderGroup, 
                    loaded_bitmap *bitmap,
                    v3 p, v3 halfDim, v4 color)
{
    v3 pixelP = (p - renderGroup->camera.p) / renderGroup->mToPixel;
    pixelP += V3(buffer->center, 0.0f);
    v3 halfDimInPixel = halfDim / renderGroup->mToPixel;

    v2 minCorner = pixelP.xy - halfDimInPixel.xy;
    v2 maxCorner = pixelP.xy + halfDimInPixel.xy;

    DrawBitmap(buffer, bitmap, minCorner, maxCorner, color);
}