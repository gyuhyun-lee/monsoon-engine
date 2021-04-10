
struct raycast_testing_segment
{
    v2 segStart;
    v2 segEnd;
};

internal b32
RaycastToSegmentResultOnly(v2 rayStart, v2 rayDir, v2 segStart, v2 segEnd)
{
    b32 result = false;

    v2 segDir = segEnd - segStart;

    r32 t2 = (rayDir.x*(segStart.y - rayStart.y) + rayDir.y*(rayStart.x - segStart.x))/
            (segDir.x*rayDir.y - segDir.y*rayDir.x);
    r32 t1 = (segStart.x+segDir.x*t2-rayStart.x)/rayDir.x;

    if(t1 > 0.0f && 
        t2 > 0.0f && t2 < 1.0f)
    {
        result = true;
    }

    return result;
}

internal b32
RaycastToNonRotatedSquareResultOnly(v2 rayStart, r32 rayRad, v2 targetP, v2 targetHalfDim)
{
    b32 result = false;

    raycast_testing_segment segments[] = 
    {
        {V2(targetP.x + targetHalfDim.x, targetP.y - targetHalfDim.y), V2(targetP.x + targetHalfDim.x, targetP.y + targetHalfDim.y)},
        {V2(targetP.x + targetHalfDim.x, targetP.y + targetHalfDim.y), V2(targetP.x - targetHalfDim.x, targetP.y + targetHalfDim.y)},
        {V2(targetP.x - targetHalfDim.x, targetP.y + targetHalfDim.y), V2(targetP.x - targetHalfDim.x, targetP.y - targetHalfDim.y)},
        {V2(targetP.x - targetHalfDim.x, targetP.y - targetHalfDim.y), V2(targetP.x + targetHalfDim.x, targetP.y - targetHalfDim.y)},
    };

    v2 rayDir = V2(Cos(rayRad), Sin(rayRad));

    for(u32 segIndex = 0;
        result == false && segIndex < ArrayCount(segments);
        ++segIndex)
    {
        raycast_testing_segment *segment = segments + segIndex;
        result = RaycastToSegmentResultOnly(rayStart, rayDir, segment->segStart, segment->segEnd);
    }

    return result;
}

struct raycast_2d_result
{
    b32 collided;
    v2 p;
};

internal raycast_2d_result
RaycastToSegment(v2 rayStart, v2 rayDir, v2 segStart, v2 segEnd)
{
    raycast_2d_result result = {};

    v2 segDir = segEnd - segStart;

    r32 t2 = (rayDir.x*(segStart.y - rayStart.y) + rayDir.y*(rayStart.x - segStart.x))/
            (segDir.x*rayDir.y - segDir.y*rayDir.x);
    r32 t1 = (segStart.x+segDir.x*t2-rayStart.x)/rayDir.x;

    if(t1 > 0.0f && 
        t2 > 0.0f && t2 < 1.0f)
    {
        result.collided = true;
        result.p = Lerp(segStart, t2, segEnd);
    }

    return result;
}

internal raycast_2d_result
RaycastToNonRotatedSquare(v2 rayStart, r32 rayRad, v2 targetP, v2 targetHalfDim)
{
    raycast_2d_result result = {};

    raycast_testing_segment segments[] = 
    {
        {V2(targetP.x + targetHalfDim.x, targetP.y - targetHalfDim.y), V2(targetP.x + targetHalfDim.x, targetP.y + targetHalfDim.y)},
        {V2(targetP.x + targetHalfDim.x, targetP.y + targetHalfDim.y), V2(targetP.x - targetHalfDim.x, targetP.y + targetHalfDim.y)},
        {V2(targetP.x - targetHalfDim.x, targetP.y + targetHalfDim.y), V2(targetP.x - targetHalfDim.x, targetP.y - targetHalfDim.y)},
        {V2(targetP.x - targetHalfDim.x, targetP.y - targetHalfDim.y), V2(targetP.x + targetHalfDim.x, targetP.y - targetHalfDim.y)},
    };

    v2 rayDir = V2(Cos(rayRad), Sin(rayRad));

    r32 distanceSquare = 10000000.0f;

    for(u32 segIndex = 0;
        segIndex < ArrayCount(segments);
        ++segIndex)
    {
        raycast_testing_segment *segment = segments + segIndex;

        raycast_2d_result segRaycastResult = RaycastToSegment(rayStart, rayDir, segment->segStart, segment->segEnd);

        if(segRaycastResult.collided)
        {
            r32 distanceBetweenSquare = LengthSq(segRaycastResult.p - rayStart);
            if(distanceSquare > distanceBetweenSquare)
            {
                distanceSquare = distanceBetweenSquare;
                result.collided = true;
                result.p = segRaycastResult.p;
            }
        }
    }

    return result;
}

