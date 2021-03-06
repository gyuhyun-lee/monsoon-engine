#include "monsoon_render.h"
#include "monsoon_simd.h"

// TODO : Texels located at the edges cannot be sampled bilinearly, so they need to be adjusted
// This function should be gone!
internal v2
NonEdgedTextureCoordForBilinearSample(v2 uv, i32 width, i32 height)
{
    return V2(uv.x*((r32)width - 2) + 1.0f, 
            uv.y*((r32)height - 2) + 1.0f);
}

internal v4
Unpack4x8(u32 value)
{
    return V4((value >> 16) & 0x000000ff, 
                (value >> 8) & 0x000000ff, 
                (value >> 0) & 0x000000ff,
                (value >> 24) & 0x000000ff); 
}

internal float32x4_t
Clamp(float32x4_t min, float32x4_t value, float32x4_t max)
{
    // TODO : These ARM instructions do not match exactly to intel instructions
    // might wanna double check later
    return vminq_f32(vmaxq_f32(min, value), max);
}

internal float32x4_t
Clamp01(float32x4_t value)
{
    float32x4_t zero = vdupq_n_f32(0.0f);
    float32x4_t one = vdupq_n_f32(1.0f);

    return Clamp(zero, value, one);
}

internal v4
sRGB255ToLinear01(v4 value)
{
    v4 result = {};

    r32 inv255 = 1/255.0f;

    value.r *= inv255;
    value.g *= inv255;
    value.b *= inv255;

    // NOTE : The correct sRGB gamma correction value is power of 2.2, 
    // but to simplify the math, we are using power of 2.
    result.r = Square(value.r);
    result.g = Square(value.g);
    result.b = Square(value.b);
    result.a = value.a/255.0f;

    return result;
}

internal v4
Linear01TosRGB255(v4 value)
{
    v4 result = {};

    result.r = 255.0f*SquareRoot2(value.r);
    result.g = 255.0f*SquareRoot2(value.g);
    result.b = 255.0f*SquareRoot2(value.b);
    result.a = 255.0f*value.a;

    return result;
}
internal v4
Unpack4x8AndsRGB255ToLinear01(u32 value)
{
    return sRGB255ToLinear01(Unpack4x8(value));
}

// NOTE : source should be pre-multiplied
internal v4
BlendPremultipliedPixels(v4 dest, v4 source)
{
    v4 result = {};

    r32 InvSourceA = 1.0f - source.a;
    result.a = dest.a*InvSourceA + source.a; 
    // NOTE : Source R, G, B are pre multiplied by the SourceA
    // TODO : Overly Overlapped pixelss make wrong color value, might wanna do something with that?
    result.r = Clamp01(dest.r*InvSourceA + source.r);
    result.g = Clamp01(dest.g*InvSourceA + source.g);
    result.b = Clamp01(dest.b*InvSourceA + source.b);

    return result;
}

struct bilinear_sampling_result
{
    v4 A;
    v4 B;
    v4 C;
    v4 D;

    r32 linearXt;
    r32 linearYt;
};
internal bilinear_sampling_result
BilinearSample(pixel_buffer_32 *buffer, v2 textureCoord)
{
    bilinear_sampling_result samplingResult;

    u32 truncatedTextureCoordX = TruncateR32ToUInt32(textureCoord.x);
    u32 truncatedTextureCoordY = TruncateR32ToUInt32(textureCoord.y);
    i32 texelOffsetX = textureCoord.x - truncatedTextureCoordX >= 0.5f ? 1 : -1;
    i32 texelOffsetY = textureCoord.y - truncatedTextureCoordY >= 0.5f ? 1 : -1;

    u32 *texelAPtr = (u32 *)buffer->memory + 
                                truncatedTextureCoordY*buffer->width +
                                truncatedTextureCoordX;

    u32 *texelBPtr = texelAPtr + texelOffsetX;
    u32 *texelCPtr = texelAPtr + texelOffsetY*buffer->width;
    u32 *texelDPtr = texelAPtr + texelOffsetX + texelOffsetY*buffer->width;

    samplingResult.A = Unpack4x8(*texelAPtr);
    samplingResult.B = Unpack4x8(*texelBPtr);
    samplingResult.C = Unpack4x8(*texelCPtr);
    samplingResult.D = Unpack4x8(*texelDPtr);

    // TODO : Simplify these linear values? Handmade hero always uses upper & right side texels. Do we want to use that?
    r32 linearXt = textureCoord.x - (r32)truncatedTextureCoordX;
    if(linearXt >= 0.5f)
    {
        linearXt -= 0.5f;
    }
    else
    {
        linearXt = 1.0f - (linearXt + 0.5f);
    }

    r32 linearYt = textureCoord.y - (r32)truncatedTextureCoordY;
    if(linearYt >= 0.5f)
    {
        linearYt -= 0.5f;
    }
    else
    {
        linearYt = 1.0f - (linearYt + 0.5f);
    }

    samplingResult.linearXt = linearXt;
    samplingResult.linearYt = linearYt;

    return samplingResult;
}

// TODO : Take blinear_sampling_result as parameter?
internal v4
BilinearBlend(v4 A, v4 B, v4 C, v4 D, r32 xt, r32 yt)
{
    return V4(LinearInterpolation(LinearInterpolation(A.r, xt, B.r), 
                                                        yt,
                                                        LinearInterpolation(C.r, xt, D.r)),
                                    LinearInterpolation(LinearInterpolation(A.g, xt, B.g), 
                                                yt,
                                                LinearInterpolation(C.g, xt, D.g)),
                                    LinearInterpolation(LinearInterpolation(A.b, xt, B.b), 
                                                yt,
                                                LinearInterpolation(C.b, xt, D.b)),
                                    LinearInterpolation(LinearInterpolation(A.a, xt, B.a), 
                                                yt,
                                                LinearInterpolation(C.a, xt, D.a)));

}

internal void
ClearPixelBuffer32(pixel_buffer_32 *buffer, v4 color = V4(0, 0, 0, 0))
{
    color.rgb *= color.a;
    u32 c = (u32)((RoundR32ToInt32(color.a * 255.0f) << 24) |
                    (RoundR32ToInt32(color.r * 255.0f) << 16) |
                    (RoundR32ToInt32(color.g * 255.0f) << 8) |
                    (RoundR32ToInt32(color.b * 255.0f) << 0));

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
            *pixel++ = c; 
        }

        row += buffer->pitch;
    }
}

// NOTE : MacOS offscreen buffer is bottom-up 
internal void
DrawRectangle(pixel_buffer_32 *buffer, v2 p, v2 xAxis, v2 yAxis, v4 color)
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
    u32 color32 = (u32)((RoundR32ToInt32(color.a * 255.0f) << 24) |
                    (RoundR32ToInt32(color.r * 255.0f) << 16) |
                    (RoundR32ToInt32(color.g * 255.0f) << 8) |
                    (RoundR32ToInt32(color.b * 255.0f) << 0));

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
                *pixel++ = color32;
            }
        }
        row += buffer->pitch;
    }
}

// TODO : A lot of mangling of pixel - meter here, 
// maybe we can clear this by pre-computing the stuff outside the rendering loop
// This also makes much more sense for multi-light reflections, where the light
// can bounce multiple times! Also with that, I can get rid of metersToPixels value totally
// NOTE : This will produce 0 to 1 linear value
internal v4
SampleFromEnvironmentMap(environment_map *envMap, 
                        v2 pixelP, v2 bitmapDim,
                        v3 reflectionVector, r32 yDiffInMeter, r32 metersToPixels)
{
    v4 result = {};

    r32 pixelsToMeters = 1.0f/metersToPixels;
    v2 pixelPInMeters = pixelsToMeters*pixelP;

    // TODO : Support for multiple LODs based on the roughness of the normal map
    pixel_buffer_32 *LOD = &envMap->LOD;
    r32 widthOverHeight = (r32)LOD->width/(r32)LOD->height;

    // TODO : For now, the origin of the environment map is always equal to the bitmap
    // that we are using. environment map really should be placed anywhere in the world!
    v3 envP = V3(0, yDiffInMeter, 0);
    v3 envHalfDim = 20*V3(widthOverHeight, 0, 1);

    r32 yDiffInPixels = envP.y*metersToPixels;

    // TODO : Somehow, my pixel p is wrong!!
    r32 castedX = (envP.y/reflectionVector.y) * reflectionVector.x; //+ pixelPInMeters.x;
    r32 castedZ = (envP.y/reflectionVector.y) * reflectionVector.z; //+ pixelPInMeters.y;

    if(castedX >= envP.x-envHalfDim.x && castedX < envP.x+envHalfDim.x &&
       castedZ >= envP.z-envHalfDim.z && castedZ < envP.x+envHalfDim.z)
    {
        r32 envMapX = (castedX - (envP.x - envHalfDim.x))/(2.0f*envHalfDim.x);
        r32 envMapZ = (castedZ - (envP.z - envHalfDim.z))/(2.0f*envHalfDim.z);

        v2 envTexelP = NonEdgedTextureCoordForBilinearSample(V2(Clamp01(envMapX), Clamp01(envMapZ)), LOD->width, LOD->height);

        bilinear_sampling_result envMapSamplingResult = BilinearSample(LOD, envTexelP);
        result = BilinearBlend(envMapSamplingResult.A,
                                envMapSamplingResult.B,
                                envMapSamplingResult.C,
                                envMapSamplingResult.D,
                                envMapSamplingResult.linearXt,
                                envMapSamplingResult.linearYt);

#if 0
        u32 *texelPtr = (u32 *)LOD->memory + 
                        (u32)envTexelP.y*LOD->width + 
                        (u32)envTexelP.x; 
        *texelPtr = 0x99ffffff;
#endif
    }

    return result;
}

// NOTE : p and the two axises are in pixel!
// NOTE : This is a simple function that just draws a fixed-sixed bitmap
// in certain position
internal void
DrawBMPSimple(pixel_buffer_32 *destBuffer, pixel_buffer_32 *sourceBuffer,
            v2 p, v2 dim)
{
    Assert(destBuffer && sourceBuffer);

    v2 p0 = p;
    v2 p1 = p + V2(dim.x, 0);
    v2 p2 = p + V2(0, dim.y);
    v2 p3 = p + dim;

    i32 minX = RoundR32ToInt32(Minimum(Minimum(p0.x, p1.x), Minimum(p2.x, p3.x)));
    i32 minY = RoundR32ToInt32(Minimum(Minimum(p0.y, p1.y), Minimum(p2.y, p3.y)));
    i32 maxX = RoundR32ToInt32(Maximum(Maximum(p0.x, p1.x), Maximum(p2.x, p3.x)));
    i32 maxY = RoundR32ToInt32(Maximum(Maximum(p0.y, p1.y), Maximum(p2.y, p3.y)));

    i32 sourceOffsetX = 0;
    i32 sourceOffsetY = 0;
    if(minX < 0) 
    {
        sourceOffsetX = -minX;
        minX = 0;
    }
    if(maxX > destBuffer->width)
    {
        maxX = destBuffer->width;
    }
    if(minY < 0)
    {
        sourceOffsetY = -minY;
        minY = 0;
    }
    if(maxY > destBuffer->height)
    {
        maxY = destBuffer->height;
    }   

    u8 *source = (u8 *)sourceBuffer->memory + 
                    sourceBuffer->pitch*sourceOffsetY + 
                    sourceBuffer->bytesPerPixel*sourceOffsetX;

    u8 *row = (u8 *)destBuffer->memory + 
                    destBuffer->pitch*minY + 
                    destBuffer->bytesPerPixel*minX;
    for(i32 y = minY;
        y < maxY;
        ++y)
    {
        u32 *sourcePixel = (u32 *)source;
        u32 *pixel = (u32 *)row;
        for(i32 x = minX;
            x < maxX;
            ++x)
        {
            *pixel++ = *sourcePixel++;
        }

        source += sourceBuffer->pitch;
        row += destBuffer->pitch;
    }
}


// NOTE : p is ALWAYS the pixel p based on the entity p.
// only the alignment can be used to adjust the bmp drawing position.
// NOTE : p and the two axises are in pixel!
internal void
DrawBMP(pixel_buffer_32 *destBuffer, pixel_buffer_32 *sourceBuffer,
        v2 p, v4 color = V4(1, 1, 1, 1), v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1),
        pixel_buffer_32 *normalMap = 0, environment_map *envMaps = 0, r32 metersToPixels = 0.0f)
{
    color.rgb *= color.a;

    r32 lengthOfXAxis = Length(xAxis);
    r32 lengthOfYAxis = Length(yAxis);
    r32 lengthOfXAxisOverYAxis = lengthOfXAxis/lengthOfYAxis;
    r32 LengthSquareOfXAxis = LengthSquare(xAxis);
    r32 LengthSquareOfYAxis = LengthSquare(yAxis);

    r32 oneOver255 = 1.0f/255.0f;

    // NOTE : When we have a non-uniform scaled axises, normals will change in scale,
    // but only the X and Y components will be affected. Therefore, if we normalize 
    // the changed normal, Z value will be overwhelmed by the scaled X and Y value
    // This value is to negate that affect as much as possible.
    r32 normalZScale = 0.5f*(lengthOfXAxis + lengthOfYAxis);

    Assert(sourceBuffer);

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

    if(minX < 0) 
    {
        minX = 0;
    }
    if(maxX > destBuffer->width)
    {
        maxX = destBuffer->width;
    }
    if(minY < 0)
    {
        minY = 0;
    }
    if(maxY > destBuffer->height)
    {
        maxY = destBuffer->height;
    }   

#if 0
    // NOTE : To see the region of the whole BMP, enable this.
    DrawRectangle(destBuffer, V2(minX, minY), dim, 1, 0, 0);
#endif

    u8 *row = (u8 *)destBuffer->memory + 
                    destBuffer->pitch*minY + 
                    destBuffer->bytesPerPixel*minX;
    for(i32 y = minY;
        y < maxY;
        ++y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = minX;
            x < maxX;
            ++x)
        {
            BeginCycleCounter(TestPixel);
            // TODO : if the p has fractional value, it might make the dot value
            // to be sligtly off(i.e -0.33333f), and the pixel can fail the test below.
            // What will be a proper way to handle this?(Round?)
            v2 v = V2(x, y) - p;
            r32 vDotXAxis = Dot(v, xAxis);
            r32 vDotYAxis = Dot(v, yAxis);

            if(vDotXAxis >= 0 && vDotYAxis >= 0 &&
                vDotXAxis <= LengthSquareOfXAxis && vDotYAxis <= LengthSquareOfYAxis)
            {
                BeginCycleCounter(FillPixel);
                v2 uv = V2(vDotXAxis/LengthSquareOfXAxis, vDotYAxis/LengthSquareOfYAxis);
                v2 textureCoord = NonEdgedTextureCoordForBilinearSample(uv, sourceBuffer->width, sourceBuffer->height);

                bilinear_sampling_result samplingResult = BilinearSample(sourceBuffer, textureCoord);

                // NOTE : Because the monitors that the artist & player are using have sRGB gamma space,
                // we have to convert them to linear space so that we can do the linear blending as before
                // and convert back to sRGB value to put inside the frame buffer.
                v4 linearTexelA = sRGB255ToLinear01(samplingResult.A);
                v4 linearTexelB = sRGB255ToLinear01(samplingResult.B);
                v4 linearTexelC = sRGB255ToLinear01(samplingResult.C);
                v4 linearTexelD = sRGB255ToLinear01(samplingResult.D);

                // TODO : When we use the sub pixel method, what should be the result A?
                v4 texel = BilinearBlend(linearTexelA, linearTexelB, linearTexelC, linearTexelD, 
                                        samplingResult.linearXt, 
                                        samplingResult.linearYt);

                v4 lightColor = V4(0, 0, 0, 0);
                if(normalMap && envMaps)
                {
                    /*
                     * About the normal map
                     * 1. We assume that the normal map has same width and height with the bitmap that we are using.
                     * 2. Y is actually the value that we should use to decide which environment map we should use.
                     * 3. X and Z will be used to decide which texel should we use inside the environment map
                     * */
                    bilinear_sampling_result norMapSamplingResult = BilinearSample(normalMap, textureCoord);

                    v4 normal = BilinearBlend(norMapSamplingResult.A, norMapSamplingResult.B, norMapSamplingResult.C, norMapSamplingResult.D,
                                                norMapSamplingResult.linearXt, norMapSamplingResult.linearYt);

                    // NOTE : put normal values back into -1 to 1 range.
                    normal.xyz = oneOver255*2.0f*normal.xyz - V3(1.f, 1.f, 1.f);

                    // NOTE : If the bitmap is not uniformly scaled, unlike the other points
                    // normals are affected reversly.
                    // For examlple, if the bitmap is scaled by two only in xAxis,
                    // the normal will be Nx*xAxis + 2*Ny*yAxis
                    normal.xy = normal.x*xAxis + 
                                lengthOfXAxisOverYAxis*normal.y*yAxis;
                    normal.z *= normalZScale;
                    
                    normal.xyz = Normalize(normal.xyz);

                    v3 Eye = V3(0, 0, 1); 
                    v3 reflectionVector = Normalize(Eye - 2.0f*Dot(Eye, normal.xyz)*normal.xyz);

                    // This is just the simplified version of the reflection -e + 2e^T N N
                    //v3 reflectionVector = 2.0f*normal.z*normal.xyz;
                    //reflectionVector.z -= 1.0f;

                    environment_map *envMapToUse = 0;
                    v4 envMapTexel = {};
                    r32 yDiffInMeter = 0.0f;

                    if(reflectionVector.y > 0.75f)
                    {
                        // NOTE : Use the upper environment majp
                        envMapToUse = envMaps + 2;
                        yDiffInMeter = 5.0f;
                    }
#if 1
                    else if(reflectionVector.y < -0.75f)
                    {
                        // NOTE : We should use the bottom map,
                        // but treat it as if it's the top env map by reversing the y value
                        //reflectionVector.y = -reflectionVector.y;
                        // NOTE : Use the bottom enviroment map
                        envMapToUse = envMaps + 0;

                        yDiffInMeter = -5.0f;
                    }
                    else
                    {
                        // middle map??
                        //envMapToUse = envMaps + 1;
                    }
#endif

                    if(envMapToUse)
                    {

                        v2 pixelP = uv.x*xAxis + uv.y*yAxis - 0.5f*V2(lengthOfXAxis, lengthOfYAxis);
                        lightColor = SampleFromEnvironmentMap(envMapToUse, 
                                                            pixelP, 
                                                            V2(lengthOfXAxis, lengthOfYAxis),
                                                            reflectionVector, yDiffInMeter,
                                                            metersToPixels);
                        lightColor = sRGB255ToLinear01(lightColor);
                    }
#if 0
                    v3 normalizedReflectionVector = Normalize(reflectionVector);
                    texel.r = 0.5f*(normalizedReflectionVector.x + 1.0f);
                    texel.g = 0.5f*(normalizedReflectionVector.y + 1.0f);
                    texel.b = 0.5f*(normalizedReflectionVector.z + 1.0f);
                    texel.r = Clamp01(texel.r);
                    texel.g = Clamp01(texel.g);
                    texel.b = Clamp01(texel.b);
                    //texel.r = 0.0f;
                    //texel.g = 0.0f;
                    texel.b = 0.0f;
                    texel.a = 1.0f;
#endif
                }

                texel.rgb = texel.rgb + texel.a*lightColor.rgb;

                texel = Hadamard(texel, color);
                texel.r = Clamp01(texel.r);
                texel.g = Clamp01(texel.g);
                texel.b = Clamp01(texel.b);

                v4 destC = Unpack4x8AndsRGB255ToLinear01(*pixel);
                v4 resultColor = {};

                r32 InvSourceA = 1.0f - texel.a;
                resultColor.a = destC.a*InvSourceA + texel.a; 
                // NOTE : Source R, G, B are pre multiplied by the SourceA
                // TODO : Overly Overlapped pixelss make wrong color value, might wanna do something with that?
                resultColor.r = Clamp01(destC.r*InvSourceA + texel.r);
                resultColor.g = Clamp01(destC.g*InvSourceA + texel.g);
                resultColor.b = Clamp01(destC.b*InvSourceA + texel.b);

                v4 resultColor255 = Linear01TosRGB255(V4(resultColor.r, resultColor.g, resultColor.b, resultColor.a));

                *pixel = (RoundR32ToUInt32(resultColor255.a) << 24 |
                        (RoundR32ToUInt32(resultColor255.r) << 16) |
                        (RoundR32ToUInt32(resultColor255.g) << 8) |
                        (RoundR32ToUInt32(resultColor255.b) << 0));
                EndCycleCounter(FillPixel);
            }

            pixel++;
            EndCycleCounter(TestPixel);
        }


        row += destBuffer->pitch;
    }
}

#define mulSquare_r32(a) vmulq_f32(a, a);
#define GSr32(r, i) *(((float32_t *)&r) + i)
#define GSu32(r, i) *(((uint32_t *)&r) + i)

// NOTE : p is ALWAYS the pixel p based on the entity p.
// only the alignment can be used to adjust the bmp drawing position.
// NOTE : p and the two axises are in pixel!
internal void
DrawBMPQuickly_ARM(pixel_buffer_32 *destBuffer, pixel_buffer_32 *sourceBuffer,
        v2 p, v4 color = V4(1, 1, 1, 1), v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))

{
    Assert(sourceBuffer);
    color.rgb *= color.a;

    float32x4_t colorr_128 = vdupq_n_f32(color.r); // _mm_set_ps
    float32x4_t colorg_128 = vdupq_n_f32(color.g); // _mm_set_ps
    float32x4_t colorb_128 = vdupq_n_f32(color.b); // _mm_set_ps
    float32x4_t colora_128 = vdupq_n_f32(color.a); // _mm_set_ps

    r32 oneOver255 = 1.0f/255.0f;
    r32 oneOver255Sq = oneOver255*oneOver255;
    
    float32x4_t v255 = vdupq_n_f32(255.0f);
    float32x4_t v255Sq = vdupq_n_f32(255.0f*255.0f);
    float32x4_t zero_128 = vdupq_n_f32(0.0f); // _mm_set_ps
    float32x4_t one_128 = vdupq_n_f32(1.0f); // _mm_set_ps
    float32x4_t oneOver255_128 = vdupq_n_f32(oneOver255); // _mm_set_ps
    float32x4_t oneOver255Sq_128 = vdupq_n_f32(oneOver255Sq); // _mm_set_ps

    uint32x4_t mask_0x000000ff = vdupq_n_u32(0x000000ff); // _mm_set_ps
    uint32x4_t mask_0x00ff00ff = vdupq_n_u32(0x00ff00ff); // _mm_set_ps
    uint32x4_t mask_0xff00ff00 = vdupq_n_u32(0xff00ff00); // _mm_set_ps

    r32 lengthOfXAxis = Length(xAxis);
    r32 lengthOfYAxis = Length(yAxis);
    r32 lengthOfXAxisOverYAxis = lengthOfXAxis/lengthOfYAxis;
    r32 LengthSquareOfXAxis = LengthSquare(xAxis);
    r32 LengthSquareOfYAxis = LengthSquare(yAxis);
    float32x4_t xAxisX = vdupq_n_f32(xAxis.x);
    float32x4_t yAxisX = vdupq_n_f32(xAxis.y);
    float32x4_t xAxisY = vdupq_n_f32(yAxis.x);
    float32x4_t yAxisY = vdupq_n_f32(yAxis.y);
    float32x4_t oneOverLengthSquareOfXAxis = vdupq_n_f32(1/LengthSquareOfXAxis);
    float32x4_t oneOverLengthSquareOfYAxis = vdupq_n_f32(1/LengthSquareOfYAxis);

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

    i32 maxDestWidth = destBuffer->width - 4;
    i32 maxDestHeight = destBuffer->height - 4;
    
    // TODO : Don't trunacte this!
    float32x4_t sourceWidth_4x = vdupq_n_f32((r32)sourceBuffer->width - 4.0f);
    float32x4_t sourceHeight_4x = vdupq_n_f32((r32)sourceBuffer->height - 4.0f);

    if(minX < 0) 
    {
        minX = 0;
    }
    if(maxX > maxDestWidth)
    {
        maxX = maxDestWidth;
    }
    if(minY < 0)
    {
        minY = 0;
    }
    if(maxY > maxDestHeight)
    {
        maxY = maxDestHeight;
    }   

    r32 originpDotXAxis = Dot(p, xAxis);
    r32 originpDotYAxis = Dot(p, yAxis);

#if 0
    // NOTE : To see the region of the whole BMP, enable this.
    DrawRectangle(destBuffer, V2(minX, minY), dim, 1, 0, 0);
#endif

    u32 *sourceBufferMemory = (u32 *)sourceBuffer->memory;
    i32 sourceBufferWidth = sourceBuffer->width;
    u8 *row = (u8 *)destBuffer->memory + 
                    destBuffer->pitch*minY + 
                    destBuffer->bytesPerPixel*minX;
    for(i32 y = minY;
        y < maxY;
        ++y)
    {
        u32 *pixel = (u32 *)row;
        for(i32 x = minX;
            x < maxX;
            x += 4)
        {
            float32_t originRelX = x - p.x;
            float32_t originRelY = y - p.y;
            float32_t xi[] = {originRelX, originRelX+1, originRelX+2, originRelX+3};
            float32x4_t px = vld1q_f32(xi);
            float32x4_t py = vdupq_n_f32(originRelY);

             // TODO : if the p has fractional value, it might make the dot value
            // to be sligtly off(i.e -0.33333f), and the pixel can fail the test below.
            // What will be a proper way to handle this?(Round?)
            // NOTE : This will also handle what the 'sourceOffset' value was doing
            float32x4_t u = vmulq_f32(vaddq_f32(vmulq_f32(px, xAxisX), vmulq_f32(py, xAxisY)), oneOverLengthSquareOfXAxis);
            float32x4_t v = vmulq_f32(vaddq_f32(vmulq_f32(px, yAxisX), vmulq_f32(py, yAxisY)), oneOverLengthSquareOfYAxis);

            // NOTE : Check whether the pixel is inside the drawing bound
            // by dotting with each axis and comparing with the length square of each axis
            uint32x4_t shouldFill = vandq_u32(vandq_u32(vcgeq_f32(u, zero_128), vcleq_f32(u, one_128)), 
                                            vandq_u32(vcgeq_f32(v, zero_128), vcleq_f32(v, one_128)));

            float32x4_t textureCoordX = vaddq_f32(vmulq_f32(u, sourceWidth_4x), one_128);
            float32x4_t textureCoordY = vaddq_f32(vmulq_f32(v, sourceHeight_4x), one_128);
            uint32x4_t truncatedTextureCoordX = vcvtq_u32_f32(textureCoordX);
            uint32x4_t truncatedTextureCoordY = vcvtq_u32_f32(textureCoordY);

            float32x4_t linearXt = vsubq_f32(textureCoordX, vcvtq_f32_u32(truncatedTextureCoordX));
            float32x4_t linearYt = vsubq_f32(textureCoordY, vcvtq_f32_u32(truncatedTextureCoordY));

            // NOTE : Bilinear Sampling
            uint32x4_t texelA;
            uint32x4_t texelB;
            uint32x4_t texelC;
            uint32x4_t texelD;
            for(u32 i = 0;
                i < 4;
                ++i)
            {
                if(GSu32(shouldFill, i))
                {
                    // NOTE : Bilinear Sampling
                    u32 *texelAPtr = sourceBufferMemory + GSu32(truncatedTextureCoordY, i)*sourceBuffer->width + GSu32(truncatedTextureCoordX, i);
                    GSu32(texelA, i) = *texelAPtr;
                    GSu32(texelB, i) = *(texelAPtr + 1);
                    GSu32(texelC, i) = *(texelAPtr + sourceBufferWidth);
                    GSu32(texelD, i) = *(texelAPtr + sourceBufferWidth + 1);
                }
            }

            texelA = vandq_u32(shouldFill, texelA);
            texelB = vandq_u32(shouldFill, texelB);
            texelC = vandq_u32(shouldFill, texelC);
            texelD = vandq_u32(shouldFill, texelD);

            // TODO : Unpack in 16bit value(R-B, G-A) to work on 8lanes simultaneously? Check the performance first,
            // and then work on it!
            float32x4_t texelAr = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelA, 16), mask_0x000000ff));
            float32x4_t texelAg = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelA, 8), mask_0x000000ff));
            float32x4_t texelAb = vcvtq_f32_u32(vandq_u32(texelA, mask_0x000000ff));
            float32x4_t texelAa = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelA, 24), mask_0x000000ff));

#if 1
            uint16x8_t texelAbr = (uint16x8_t)vandq_u32(texelA, mask_0x00ff00ff);
            uint16x8_t texelAag = (uint16x8_t)vandq_u32(texelA, mask_0xff00ff00);
#else
            uint32x4_t texelArb = vandq_u32(texelA, mask_0x00ff00ff);`
            uint32x4_t texelAga = vandq_u32(texelA, mask_0xff00ff00);
#endif

            float32x4_t texelBr = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelB, 16), mask_0x000000ff));
            float32x4_t texelBg = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelB, 8), mask_0x000000ff)); float32x4_t texelBb = vcvtq_f32_u32(vandq_u32(texelB, mask_0x000000ff));
            float32x4_t texelBa = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelB, 24), mask_0x000000ff));

            float32x4_t texelCr = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelC, 16), mask_0x000000ff));
            float32x4_t texelCg = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelC, 8), mask_0x000000ff));
            float32x4_t texelCb = vcvtq_f32_u32(vandq_u32(texelC, mask_0x000000ff));
            float32x4_t texelCa = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelC, 24), mask_0x000000ff));

            float32x4_t texelDr = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelD, 16), mask_0x000000ff));
            float32x4_t texelDg = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelD, 8), mask_0x000000ff));
            float32x4_t texelDb = vcvtq_f32_u32(vandq_u32(texelD, mask_0x000000ff));
            float32x4_t texelDa = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(texelD, 24), mask_0x000000ff));

            // NOTE : Convert from sRGB255 to 0 to 255^2 space(except alpha value)
            texelAr = mulSquare_r32(texelAr);
            texelAg = mulSquare_r32(texelAg);
            texelAb = mulSquare_r32(texelAb);

            texelBr = mulSquare_r32(texelBr);
            texelBg = mulSquare_r32(texelBg);
            texelBb = mulSquare_r32(texelBb);

            texelCr = mulSquare_r32(texelCr);
            texelCg = mulSquare_r32(texelCg);
            texelCb = mulSquare_r32(texelCb);

            texelDr = mulSquare_r32(texelDr);
            texelDg = mulSquare_r32(texelDg);
            texelDb = mulSquare_r32(texelDb);

            float32x4_t invLinearXt = vsubq_f32(one_128, linearXt);
            float32x4_t invLinearYt = vsubq_f32(one_128, linearYt);

            // NOTE : Interpolate Linear to get the blended color
            float32x4_t texelr = vaddq_f32(
                                vaddq_f32(vmulq_f32(vmulq_f32(invLinearYt, invLinearXt), texelAr), vmulq_f32(vmulq_f32(invLinearYt, linearXt), texelBr)), 
                                vaddq_f32(vmulq_f32(vmulq_f32(linearYt, invLinearXt), texelCr), vmulq_f32(vmulq_f32(linearYt, linearXt), texelDr))
                                );

            float32x4_t texelg = vaddq_f32(
                                vaddq_f32(vmulq_f32(vmulq_f32(invLinearYt, invLinearXt), texelAg), vmulq_f32(vmulq_f32(invLinearYt, linearXt), texelBg)), 

                                vaddq_f32(vmulq_f32(vmulq_f32(linearYt, invLinearXt), texelCg), vmulq_f32(vmulq_f32(linearYt, linearXt), texelDg))
                                );

            float32x4_t texelb = vaddq_f32(
                                vaddq_f32(vmulq_f32(vmulq_f32(invLinearYt, invLinearXt), texelAb), vmulq_f32(vmulq_f32(invLinearYt, linearXt), texelBb)), 
                                vaddq_f32(vmulq_f32(vmulq_f32(linearYt, invLinearXt), texelCb), vmulq_f32(vmulq_f32(linearYt, linearXt), texelDb))
                                );

            float32x4_t texela = vaddq_f32(
                                vaddq_f32(vmulq_f32(vmulq_f32(invLinearYt, invLinearXt), texelAa), vmulq_f32(vmulq_f32(invLinearYt, linearXt), texelBa)), 
                                vaddq_f32(vmulq_f32(vmulq_f32(linearYt, invLinearXt), texelCa), vmulq_f32(vmulq_f32(linearYt, linearXt), texelDa))
                                );
            
            // NOTE : Hadamard with color value
            // (because the color are 0 to 1 space, even if the texel rgb is in 0 to 255^2 space, it doesn't matter)
            texelr = vmulq_f32(texelr, colorr_128);
            texelg = vmulq_f32(texelg, colorg_128);

            // NOTE : Clamp r, g, b
            texelr = Clamp(zero_128, texelr, v255Sq);

            texelb = vmulq_f32(texelb, colorb_128);
            texela = vmulq_f32(texela, colora_128);
            texelg = Clamp(zero_128, texelg, v255Sq); 
            texelb = Clamp(zero_128, texelb, v255Sq);

            // NOTE : Load dest pixels
            uint32x4_t dest = vld1q_u32(pixel);

            // NOTE : Divide the dest pixels to r, g, b, a
            float32x4_t destr = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(dest, 16), mask_0x000000ff));
            float32x4_t destg = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(dest, 8), mask_0x000000ff));
            float32x4_t destb = vcvtq_f32_u32(vandq_u32(dest, mask_0x000000ff));
            float32x4_t desta = vcvtq_f32_u32(vandq_u32(vshrq_n_u32(dest, 24), mask_0x000000ff));
            // NOTE : Convert dest texel from sRGB to 0-255^2 space
            destr = mulSquare_r32(destr);
            destg = mulSquare_r32(destg);
            destb = mulSquare_r32(destb);

            // NOTE : Blend between texel and dest texel
            float32x4_t invSourceA = vsubq_f32(one_128, vmulq_f32(oneOver255_128, texela)); // NOTE : Source R, G, B are pre multiplied by the SourceA
            float32x4_t resultr = Clamp(zero_128, vaddq_f32(vmulq_f32(destr, invSourceA), texelr), v255Sq);
            float32x4_t resultg = Clamp(zero_128, vaddq_f32(vmulq_f32(destg, invSourceA), texelg), v255Sq);
            float32x4_t resultb = Clamp(zero_128, vaddq_f32(vmulq_f32(destb, invSourceA), texelb), v255Sq);
            float32x4_t resulta = Clamp(zero_128, vaddq_f32(vmulq_f32(desta, invSourceA), texela), v255);

            // NOTE : Convert result from linear 01 to sRGB255
            resultr = vsqrtq_f32(resultr);
            resultg = vsqrtq_f32(resultg);
            resultb = vsqrtq_f32(resultb);

            // NOTE : Because we know that the result value is in 0~255 range,
            // we don't have to mask them before shifting left
            uint32x4_t resultIntR = vshlq_n_u32(vcvtq_u32_f32(resultr), 16);
            uint32x4_t resultIntG = vshlq_n_u32(vcvtq_u32_f32(resultg), 8);
            uint32x4_t resultIntB = vcvtq_u32_f32(resultb);
            uint32x4_t resultIntA = vshlq_n_u32(vcvtq_u32_f32(resulta), 24);

            uint32x4_t result = vorrq_u32(vorrq_u32(resultIntR, resultIntG), vorrq_u32(resultIntB, resultIntA));

            // TODO : ShouldFill!
            
            vst1q_u32(pixel, result);

            pixel += 4;
        }

        row += destBuffer->pitch;
    }
}

internal render_group_camera
GetRenderGroupCamera(r32 cameraZ, r32 focalLength, r32 monitorWidthInMeter, i32 destBufferWidth, i32 destBufferHeight)
{
    render_group_camera result = {};

    result.z = cameraZ;
    result.focalLength = focalLength;

    // TODO : Handmade hero uses destBufferWidth*monitorWidthInMeter, why??
    result.metersToPixels = (r32)destBufferWidth/monitorWidthInMeter;

    result.projectedMonitorDim.x = ((destBufferWidth/result.metersToPixels)*(result.z))/
                                        focalLength;
    result.projectedMonitorDim.y = ((destBufferHeight/result.metersToPixels)*(result.z))/
                                        focalLength;

    return result;
}

internal void
StartRenderGroup(render_group *renderGroup, memory_arena *arena, memory_index renderMemorySize,
                i32 destBufferWidth, i32 destBufferHeight)
{
    // TODO : How can I keep track of used memory without explicitly mentioning it?
    renderGroup->renderMemory = StartTemporaryMemory(arena, renderMemorySize);
    renderGroup->bufferHalfDim = 0.5f*V2(destBufferWidth, destBufferHeight);

    renderGroup->renderCamera = GetRenderGroupCamera(20.0f, 0.6f, 0.635f, 
                                                    destBufferWidth, destBufferHeight);
    //renderGroup->gameCamera = GetRenderGroupCamera(20.0f, 0.6f, 0.635f, 
     //                                               destBufferWidth, destBufferHeight);
    renderGroup->gameCamera = renderGroup->renderCamera;
}

internal v2
Project(v2 p, r32 focalLength, r32 cameraToTarget)
{
    return (focalLength/cameraToTarget)*(p);
}

struct render_element_pixel_basis
{
    // NOTE : These are 
    v2 p;
    v2 xAxis;
    v2 yAxis;
};

internal render_element_pixel_basis
GetRenderElementPixelBasis(render_group *renderGroup, render_group_camera *camera, 
                        v3 p, v3 dim, v2 xAxis, v2 yAxis, v2 alignment, v2 outputBufferHalfDim)
{
    render_element_pixel_basis basis = {};

    r32 cameraToTarget = camera->z - p.z;
    v2 perspectiveP = Project(p.xy - alignment, camera->focalLength, cameraToTarget);
    v2 perspectiveDim = Project(dim.xy, camera->focalLength, cameraToTarget);

    basis.xAxis = camera->metersToPixels*perspectiveDim.x*xAxis;
    basis.yAxis = camera->metersToPixels*perspectiveDim.y*yAxis;
    // NOTE : So this assumes that center of the render group = center of the sim region
    basis.p = outputBufferHalfDim + camera->metersToPixels*perspectiveP - 
                (0.5f*basis.xAxis + 0.5f*basis.yAxis);

    return basis;
}


internal void
PushRect(render_group *renderGroup, v3 p, v3 dim, v4 color, v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1))
{
    u8 *Memory = PushSize(&renderGroup->renderMemory, sizeof(render_element_header) + sizeof(render_element_rect));
    render_element_header *header = (render_element_header *)Memory;
    render_element_rect *element = (render_element_rect *)(Memory + sizeof(render_element_header));

    header->p = p;
    header->dim = dim;

    header->xAxis = xAxis;
    header->yAxis = yAxis;

    header->type = RenderElementType_Rect;
    header->color = color;

    renderGroup->elementCount++;
}

// TODO : Allow xAxis and yAxis not to be perpendicular? -> 
// If we do allow them to be not perpendicular, the routine we are using for checking whether the pixel is inside the boundary
// or not will be invalid
internal void
PushBMP(render_group *renderGroup, pixel_buffer_32 *sourceBuffer, v3 p, v3 dim, 
        v4 color = V4(1, 1, 1, 1),
        v2 xAxis = V2(1, 0), v2 yAxis = V2(0, 1), 
        pixel_buffer_32 *normalMap = 0, environment_map *envMaps = 0)
{
    u8 *Memory = PushSize(&renderGroup->renderMemory, sizeof(render_element_header) + sizeof(render_element_bmp));
    render_element_header *header = (render_element_header *)Memory;
    render_element_bmp *element = (render_element_bmp *)(Memory + sizeof(render_element_header));

    header->p = p;
    header->dim = dim;

    header->xAxis = xAxis;
    header->yAxis = yAxis;

    header->type = RenderElementType_BMP;
    header->color = color;

    element->alignment.x = dim.x*sourceBuffer->alignPercentage.x;
    element->alignment.y = dim.y*sourceBuffer->alignPercentage.y;
    element->envMaps = envMaps;
    element->normalMap = normalMap;
    element->sourceBuffer = sourceBuffer;

    renderGroup->elementCount++;
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

// NOTE : Only this function is responsible for converting the meter to pixels!!
internal void
RenderRenderGroup(render_group *renderGroup, pixel_buffer_32 *destBuffer)
{
    u8 *Base = renderGroup->renderMemory.base;
    
    render_group_camera *camera = &renderGroup->renderCamera;

    for(u32 ElementIndex = 0;
        ElementIndex < renderGroup->elementCount;
        ++ElementIndex)
    {
        render_element_header *header = (render_element_header *)Base;
        Base += sizeof(*header);
        switch(header->type)
        {
#if 1
            case RenderElementType_Rect :
            {
                render_element_rect *element = (render_element_rect *)Base;

                render_element_pixel_basis basis = 
                    GetRenderElementPixelBasis(renderGroup, camera, header->p, header->dim, header->xAxis, header->yAxis, 
                                            V2(0, 0),
                                            renderGroup->bufferHalfDim);
                
                DrawRectangle(destBuffer, basis.p, basis.xAxis, basis.yAxis,
                                header->color);
                Base += sizeof(*element);
            }break;
#endif

            case RenderElementType_BMP :
            {
                render_element_bmp *element = (render_element_bmp *)Base;

                render_element_pixel_basis basis = 
                    GetRenderElementPixelBasis(renderGroup, camera, header->p, header->dim, header->xAxis, header->yAxis, 
                                            element->alignment,
                                            renderGroup->bufferHalfDim);
                
                DrawBMPQuickly_ARM(destBuffer, element->sourceBuffer,
                        basis.p, header->color, basis.xAxis, basis.yAxis);

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
MakeEmptyPixelBuffer32(memory_arena *arena, i32 width, i32 height, v2 alignPercentage = V2(0, 0),
                        v4 color = V4(0, 0, 0, 0))
{
    pixel_buffer_32 buffer = {};

    buffer.width = width;
    buffer.height = height;
    buffer.bytesPerPixel = 4;
    buffer.pitch = buffer.bytesPerPixel * buffer.width;
    buffer.alignPercentage = alignPercentage;
    buffer.memory = (u32 *)PushArray(arena, u32, width*height);

    ClearPixelBuffer32(&buffer, color);

    return buffer;
}

