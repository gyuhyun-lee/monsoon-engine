#include "monsoon_render.h"

// TODO : Premultiplied alpha
internal void
DrawBMP(game_offscreen_buffer *Buffer, debug_loaded_bmp *LoadedBMP, 
        v2 P, v2 Dim, v2 Alignment)
{
    P.X -= Alignment.X;
    P.Y -= Alignment.Y;
    i32 MinX = RoundR32ToInt32(P.X);
    i32 MinY = RoundR32ToInt32(P.Y);
    // TODO : Bitmap sampling based on the entity dim!
    i32 MaxX = RoundR32ToInt32(P.X + LoadedBMP->Width);
    i32 MaxY = RoundR32ToInt32(P.Y + LoadedBMP->Height);

    i32 SourceOffsetX = 0;
    i32 SourceOffsetY = 0;
    
    if(MinX < 0) 
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }   

#if 0
    // NOTE : To see the region of the whole BMP, enable this.
    DrawRectangle(Buffer, X, Y, LoadedBMP->Width, LoadedBMP->Height, 1, 0, 0);
#endif

    u8 *Row = (u8 *)Buffer->Memory + 
                    Buffer->Pitch*MinY + 
                    Buffer->BytesPerPixel*MinX;
    u8 *SourceRow = (u8 *)LoadedBMP->Pixels + 
                    LoadedBMP->Pitch*SourceOffsetY + 
                    LoadedBMP->BytesPerPixel*SourceOffsetX;

    for(i32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        u32 *SourcePixel = (u32 *)SourceRow;
        for(i32 X = MinX;
            X < MaxX;
            ++X)
        {
            // NOTE : Bit pattern for the pixel : AARRGGBB
            r32 DestR = ((*Pixel >> 16) & 0x000000ff) / 255.0f;
            r32 DestG = ((*Pixel >> 8) & 0x000000ff) / 255.0f;
            r32 DestB = ((*Pixel >> 0) & 0x000000ff) / 255.0f;

            // NOTE : Bit pattern for the BMP : AARRGGBB
            r32 SourceR = ((*SourcePixel >> 16) & 0x000000ff) / 255.0f;
            r32 SourceG = ((*SourcePixel >> 8) & 0x000000ff) / 255.0f;
            r32 SourceB = ((*SourcePixel >> 0) & 0x000000ff) / 255.0f;
            r32 SourceA = ((*SourcePixel >> 24) & 0x000000ff) / 255.0f;

            r32 ResultR = (1.0f-SourceA)*DestR + SourceA*SourceR;
            r32 ResultG = (1.0f-SourceA)*DestG + SourceA*SourceG;
            r32 ResultB = (1.0f-SourceA)*DestB + SourceA*SourceB;

            u32 ResultColor = ((u32)(ResultR*255.0f + 0.5f) << 16) |
                                ((u32)(ResultG*255.0f + 0.5f) << 8) |
                                ((u32)(ResultB*255.0f + 0.5f) << 0);

            *Pixel++ = ResultColor;
            SourcePixel++;
        }

        Row += Buffer->Pitch;
        SourceRow += LoadedBMP->Pitch;
    }
}


// NOTE : MacOS offscreen buffer is bottom-up 
internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 P, v2 Dim,
            r32 R, r32 G, r32 B)
{    
    i32 MinX = RoundR32ToInt32(P.X);
    i32 MinY = RoundR32ToInt32(P.Y);
    i32 MaxX = RoundR32ToInt32(P.X + Dim.X);
    i32 MaxY = RoundR32ToInt32(P.Y + Dim.Y);

    i32 DEBUGMinX = RoundR32ToInt32(P.X);
    i32 DEBUGMinY = RoundR32ToInt32(P.Y);
    i32 DEBUGMaxX = RoundR32ToInt32(P.X + Dim.X);
    i32 DEBUGMaxY = RoundR32ToInt32(P.Y + Dim.Y);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
 
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }   

    // NOTE : Bit pattern for the pixel : AARRGGBB
    u32 Color = (u32)((RoundR32ToInt32(R * 255.0f) << 16) |
                (RoundR32ToInt32(G * 255.0f) << 8) |
                (RoundR32ToInt32(B * 255.0f) << 0));

    u8 *Row = (u8 *)Buffer->Memory + 
                    Buffer->Pitch*MinY + 
                    Buffer->BytesPerPixel*MinX;
    for(i32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i32 X = MinX;
            X < MaxX;
            ++X)
        {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

internal void
PushRect(render_group *RenderGroup, v2 P, v2 Dim, v3 Color)
{
    u8 *Memory = PushSize(&RenderGroup->Arena, sizeof(render_element_header) + sizeof(render_element_rect));
    render_element_header *Header = (render_element_header *)Memory;
    render_element_rect *Element = (render_element_rect *)(Memory + sizeof(render_element_header));

    v2 PixelDim = RenderGroup->MetersToPixels*Dim;
    v2 PixelP = RenderGroup->BufferHalfDim + RenderGroup->MetersToPixels*P - 0.5f*PixelDim;

    Header->Type = RenderElementType_Rect;
    Header->P = PixelP;
    Element->Dim = PixelDim;
    Element->Color = Color;

    RenderGroup->ElementCount++;
}

internal void
PushBMP(render_group *RenderGroup, debug_loaded_bmp *LoadedBMP, v2 P, v2 Dim)
{
    u8 *Memory = PushSize(&RenderGroup->Arena, sizeof(render_element_header) + sizeof(render_element_bmp));
    render_element_header *Header = (render_element_header *)Memory;
    render_element_bmp *Element = (render_element_bmp *)(Memory + sizeof(render_element_header));

    v2 PixelDim = RenderGroup->MetersToPixels*Dim;
    v2 PixelP = RenderGroup->BufferHalfDim + RenderGroup->MetersToPixels*P - 0.5f*PixelDim;

    Header->Type = RenderElementType_BMP;
    Header->P = PixelP;
    Element->Dim = PixelDim;

    Element->LoadedBMP = *LoadedBMP;

    RenderGroup->ElementCount++;
}

// TODO : Better buffer clearing function
internal void 
ClearBuffer(game_offscreen_buffer *Buffer)
{
    u8 *FirstPixelOfRow = (u8 *)Buffer->Memory;
    for(int Row = 0;
        Row < Buffer->Height;
        ++Row)
    {
        u32 *Pixel = (u32 *)(FirstPixelOfRow);
        for(int Column = 0;
            Column < Buffer->Width;
            ++Column)
        {
            *Pixel++ = 0x00000000;
        }

        FirstPixelOfRow += Buffer->Pitch;
    }
}

internal void
RenderRenderGroup(render_group *RenderGroup, game_offscreen_buffer *Buffer)
{
    ClearBuffer(Buffer);

    u8 *Base = RenderGroup->Arena.Base;
    for(u32 ElementIndex = 0;
        ElementIndex < RenderGroup->ElementCount;
        ++ElementIndex)
    {
        render_element_header *Header = (render_element_header *)Base;
        Base += sizeof(*Header);
        switch(Header->Type)
        {
            case RenderElementType_Rect :
            {
                render_element_rect *Element = (render_element_rect *)Base;
                DrawRectangle(Buffer, Header->P, Element->Dim, 
                                Element->Color.X, Element->Color.Y, Element->Color.Z);
                Base += sizeof(*Element);
            }break;

            case RenderElementType_BMP :
            {
                render_element_bmp *Element = (render_element_bmp *)Base;
                DrawBMP(Buffer, &Element->LoadedBMP, 
                        // TODO : Bitmap resize based on the entity dim!!
                        Header->P, V2(Element->LoadedBMP.Width, Element->LoadedBMP.Height),
                        Element->LoadedBMP.Alignment);
                Base += sizeof(*Element);
            }break;
        }

    }
}

