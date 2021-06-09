#ifndef MONSOON_H
#define MONSOON_H

struct game_state
{
    world World;
    i32 XOffset;
    i32 YOffset;

    world_position CameraPos;

    low_entity Entities[10000];
    u32 EntityCount;

    // TODO : Get rid of this player pointer
    low_entity *Player;
    v2 dPlayer;

    debug_loaded_bmp HeadBMP;
    debug_loaded_bmp CapeBMP;
    debug_loaded_bmp TorsoBMP;

    debug_loaded_bmp RockBMP[4];
    debug_loaded_bmp GrassBMP[2];

    memory_arena WorldArena;
    
    b32 IsInitialized;
};

#endif
