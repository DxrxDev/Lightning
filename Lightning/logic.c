#define RAYMATH_IMPLEMENTATION

#include "logic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ECS_def {
    bool *entities;
    uint32_t maxentities;

    Data_t *components;
    ComponentDefine *definitions;
    uint32_t componentcount;
    
    bool *dictionary; // list of active components per entity
} ECS_def;

typedef struct BADef {
    Data_t data;
    uint32_t stride;
    uint32_t count;
} BADef;

nodisc ECS_t ECS_Create( size_t entitycount, ComponentDefine *components ){
    size_t numofcomp = 0;
    if (components){
        for (uint32_t i = 0; components[i].name != 0; ++i){
            ++numofcomp;
        }
    }

    ECS_t ret = malloc(sizeof(ECS_def));
    ret->maxentities = entitycount;
    ret->entities = calloc(sizeof(bool), entitycount);

    ret->componentcount = numofcomp;
    ret->components = malloc( sizeof(void*) * numofcomp );
    ret->definitions = malloc(sizeof(ComponentDefine) * numofcomp);
    ret->dictionary = calloc(sizeof(bool), entitycount * numofcomp);

    for (uint32_t i = 0; i < numofcomp; ++i){
        ret->components[i] = malloc(sizeof(components[i].size) * entitycount);
        ret->definitions[i] = components[i];
    }

    return ret;
}
void ECS_Destroy( ECS_t ecs ){
    for (uint32_t i = 0; i < ecs->componentcount; ++i){
        free( ecs->components[i] );
    }
    free(ecs->components);
    free( ecs );
}
Comp_t ECS_GetComp( ECS_t ecs, const char *name ){
    Comp_t ret = 0;
    for (; ret < ecs->componentcount; ++ret){
        if (!strcmp(name, ecs->definitions[ret].name)){
            return ret;
        }
    }
    return UINT32_MAX;
}

Entity_t ECS_AddEntity( ECS_t ecs ){
    Entity_t ret = 0;
    for (; ret < ecs->maxentities; ++ret){
        if (!ecs->entities[ret]){
            ecs->entities[ret] = true;
            return ret;
        }
    }
    return UINT32_MAX;
}
void ECS_RemoveEntity( ECS_t ecs, Entity_t entity ){
    if (!ECS_Exists(ecs, entity)) return;

    bool *at = ecs->dictionary + (entity * ecs->componentcount);
    for (uint32_t i = 0; i < ecs->componentcount; ++i){
        if (at[i]){
            
        }
    }

    ecs->entities[entity] = false;
}
nodisc bool  ECS_Exists( ECS_t ecs, Entity_t entity ){
    return ecs->entities[entity];
}
void ECS_AddComp( ECS_t ecs, Entity_t entity, Comp_t comp, Data_t data ){
    if (ECS_Has(ecs, entity, comp) || comp == UINT32_MAX) ExitOnError("Entity already has component, or invalid component");
    ecs->dictionary[(entity * ecs->componentcount) + comp] = true;

    if (data != 0){
        ecs->components[comp] = memcpy(ecs->components[comp], data, ecs->definitions[comp].size);
    }
}
void ECS_RemoveComp( ECS_t ecs, Entity_t entity, Comp_t comp ){
    
}
nodisc bool  ECS_Has( ECS_t ecs, Entity_t entity, Comp_t comp ){
    if (!ECS_Exists(ecs, entity) || comp == UINT32_MAX) return false;
    bool *at = ecs->dictionary + ((entity * ecs->componentcount) + comp);
    return *at;
}
nodisc Data_t ECS_Get( ECS_t ecs, Entity_t entity, Comp_t comp ){
    if (!ECS_Has(ecs, entity, comp)) ExitOnError("Trying to get unavailable component (1)");
    return ecs->components[comp] + (entity * ecs->definitions[comp].size);
}
nodisc Data_t ECS_Get0( ECS_t ecs, Comp_t comp ){
    if (comp == UINT32_MAX) ExitOnError("Trying to get unavailable component (2)");
    return ecs->components[comp];
}


nodisc BA BACreate( BADefine def ){
    BA ret = malloc( sizeof (BADef) );

    ret->stride = def.datasize;
    ret->count = def.datacount;

    ret->data = malloc( ret->stride * ret->count );

    return ret;
}
void BADestroy( BA comarr ){
    free( comarr->data );
    comarr->stride = 0;
    comarr->count = 0;

    free( comarr );
}

nodisc uint32_t BAGetStride( BA comarr ){
    return comarr->stride;
}
void BASet( BA comarr, Data_t data, uint32_t count, uint32_t at ){
    if ((at + count) >= comarr->count) return;
    memcpy( comarr->data + at, data, count * comarr->stride );
}
nodisc Data_t BaGetPointer( BA comarr, uint32_t at ){
    return comarr->data + (at * comarr->stride);
}
