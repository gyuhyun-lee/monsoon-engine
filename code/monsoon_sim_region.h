#ifndef MONSOON_SIM_REGION_H
#define MONSOON_SIM_REGION_H

struct sim_entity
{
    low_entity *lowEntity;

    v3 p;
    // TODO : These values can be accessed through the low entity, too. 
    // might wanna clean this?
    v3 dP;
    v3 dim;
    entity_type type;
};

struct sim_region
{
    sim_entity entities[1024];
    u32 entityCount;

    world_position center;
    v3 halfDim;
};

#endif
