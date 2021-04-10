#ifndef FOX_ACTIVE_REGION_H
#define FOX_ACTIVE_REGION_H


enum entity_flag
{
    entity_flag_collidable = 1,
    entity_flag_movable = 2,
    entity_flag_ysupported = 4,
    entity_flag_damageOnCollide = 8,
};
enum entity_type
{
    entity_type_null,
    entity_type_player,
    entity_type_block,
    entity_type_familiar,
    entity_type_jumper,
    entity_type_moon,
    entity_type_enemy,
    entity_type_missile,
};

struct particle
{
    v3 p;
    v3 v;
    v3 a;
    r32 lifeTime;
    r32 maxLifeTime;
};

struct particle_emitter
{
    rt3 emitArea;

    r32 angle;
    r32 angleRange; // +- 

    r32 speed;
    r32 speedRange;

    r32 life;
    r32 lifeRange;

    v3 halfDim;
    v3 halfDimRange;

    particle particles[64]; // TODO : Seperate the particles from the emitter?
};
// TODO : More collision polygons
struct collision_polygon
{
    v3 offset; // Offset from entity position
    v3 halfDim;
};

struct collision_group
{
    u32 polygonCount;
    collision_polygon *polygons;// TODO : For now, there's only one type of colliadable polygon : which is rectangle. Add more various types of polygons(or some kind of wall system)
};

struct movement_info
{
    b32 shouldddpNormalized;
    v2 speed;
    r32 drag;
    r32 inputNullDrag;
};

struct entity_status
{
    i32 priority;
    r32 duration;
};

struct entity_status_stack
{
    entity_status statuses[5];
    u32 statusCount;
};

// internal void
// UppermostStatusStack(entity_status_stack *statusStack)

// internal void
// AddNewStack(entity_status_stack *statusStack, i32 priority, r32 duration)
// {
//     if(statusStack->statusCount == 0)
//     {
//         entity_status *status = statusStack->statuses + statusStack->statusCount++;
//         status->priority = priority;
//         status->duration = duration;
//     }
//     else 
//     {
//         entity_status *uppermostStatus = statusStack->statusStack + statusStack->statusCount - 1;

//         if(uppermostStatus->priority <= priority)
//         {
//             uppermostStatus->
//         }
//     }
// }

struct active_entity;

struct active_entity
{
    entity_type type;
    u32 ID;
    u8 flags;

    entity_status_stack statusStack;

    v3 p;
    v3 v;
    
    v3 halfDim;
    collision_group *collisionGroup;

    r32 angle; // position based angle

    v3 center;
    r32 centerBasedAngle;
    r32 radius;

    r32 facingAngle;

    i32 hp;
    i32 jumpRemaining;
    particle_emitter particleEmitter; // TODO : Not every entity needs particle.

    // active_entity *sword;
};

struct active_region
{
	active_entity aes[256];
	u32 aeCount;

    world_p center;
    v3 halfDim;
};

#endif