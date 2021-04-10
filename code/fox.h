/* 
 
Author : Gyuhyeon, Lee 
Email : weanother@gmail.com 
Copyright by GyuHyeon, Lee. All Rights Reserved. 
 
*/

#ifndef FOX_H

#include "fox_platform_independent.h"
#include "fox_intrinsic.h"
#include "fox_math.h"
#include "fox_keycode.h"
#include "fox_random.h"
#include "fox_world.h"
#include "fox_active_region.h"
#include "fox_render.h"

struct sleeping_entity;

/*
    worldChunkSizeX : How many chunks are horizontaly
    worldChunkSizeY : How many chunks are vertically

    chunkSizeX, chunkSizeY : How big is the chunk in meter
*/
struct sleeping_entity
{
    world_p worldP;
    active_entity ae;
};

enum player_status_priority
{
    status_idle,
    status_move,
    status_dashAttack,
};

struct player_status
{
    i32 priority;
};

struct player_status_stack
{
    player_status stacks[5];
    u32 size;
};

internal b32
CheckUpperMostStatusStack(player_status_stack *statusStack, i32 priority)
{
    b32 result = 0;

    if(priority >= statusStack->stacks[statusStack->size].priority)
    {
        result = true;
    }

    return result;
}

internal void
AddStatusStack(player_status_stack *statusStack, i32 priority)
{
    player_status *upperMostStack = statusStack->stacks + statusStack->size;

    if(priority > upperMostStack->priority)
    {
        upperMostStack->priority = priority;
    }
    else if(priority == upperMostStack->priority)
    {
        player_status *newStack = statusStack->stacks + statusStack->size++;
        newStack->priority = priority;
    }
}

internal void
ResetStatusStack(player_status_stack *statusStack)
{
    statusStack->size = 0;
    statusStack->stacks[0].priority = status_idle;
}

struct controlled_player
{
    // This should really be inside the active entity
    // So that active entity can make/remove new status.
    player_status_stack statusStack;

    v3 ddp;
    r32 angle;
    v2 speed;
    r32 drag;

    b32 attackKeyPressed;
    r32 attackCoolDown;

    b32 semiAttackKeyPressed;
    r32 semiAttackCoolDown;

    b32 jumpKeyPressed;
    r32 jumpCoolDown;
    b32 dashKeyPressed;
    r32 damageOnCollideTime;
};

struct game_state
{
    game_world world;
    sleeping_entity ses[1024];
    u32 seCount;

    u32 cameraFollowingEntityID;

    loaded_bitmap bitmap;

    memory_arena gameArena; // TODO : More organized arenas
    collision_group *playerCollisionGroup;
    collision_group *enemyCollisionGroup;
    collision_group *blockCollisionGroup;
    collision_group *verticalBlockCollisionGroup;
    collision_group *jumperCollisionGroup;
    collision_group *moonCollisionGroup;

    render_camera camera;
    controlled_player player;

    r32 dashDuration;
    r32 maxDashDuration;
    r32 dashAngle;
    v3 dashP;

	b32 isInitialized;
};

inline sleeping_entity *
GetSleepingEntity(game_state *gameState, u32 ID)
{
    // TODO : More complex ID system?
    return gameState->ses + ID;
}

struct transient_state
{
	b32 isInitialized;
};

#define FOX_H
#endif

// Well.. the problem is that I can sure make the square vs line collision,
// but how can I do this with animation system?
