#ifndef MONSOON_SIM_REGION_H
#define MONSOON_SIM_REGION_H

struct sim_entity
{
    low_entity *LowEntity;

    v2 P;
    // TODO : These values can be accessed through the low entity, too. 
    // might wanna clean this?
    v2 dP;
    v2 Dim;
    entity_type Type;
};

struct sim_region
{
    sim_entity Entities[1024];
    u32 EntityCount;

    world_position Center;
    v2 HalfDim;
};

#endif
