#ifndef MONSOON_H
#define MONSOON_H

struct game_state
{
    // TODO : Instead of putting world inside gameState, use world arena!
    world world;

    world_position cameraPos;

    low_entity entities[10000];
    u32 entityCount;

    // TODO : Get rid of this player pointer
    low_entity *player;

    pixel_buffer_32 headBMP;
    pixel_buffer_32 capeBMP;
    pixel_buffer_32 torsoBMP;

    pixel_buffer_32 treeBMP;
    pixel_buffer_32 rockBMP[4];
    pixel_buffer_32 grassBMP[2];
    pixel_buffer_32 groundBMP[4];
    pixel_buffer_32 sampleBMP;

    // IMPORTANT : NOTE : memory arenas' temporary memory should be cleared to zero
    // at the end of the loop
    memory_arena worldArena;
    memory_arena renderArena;
    pixel_buffer_32 backgroundBuffer;
    
    r32 theta;

    // NOTE : 0 is bottom, 1 is middle, and 2 is upper map
    environment_map envMaps[3];
    pixel_buffer_32 testDiffuse;
    pixel_buffer_32 sphereNormalMap;

    // TODO : Is this a right place to store the final buffer?
    pixel_buffer_32 finalBuffer;

    b32 isInitialized;
};

#endif
