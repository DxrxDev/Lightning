#if !defined(__LOGIC_H)
#define      __LOGIC_H

#include "base.h"
#include "maths.h"

typedef void *Data_t;

/* ECS */
typedef struct ECS_def *ECS_t;
typedef uint32_t        Comp_t;
typedef uint32_t        Entity_t;
typedef void (* ComponentCleanup)( Data_t );

typedef struct ComponentDefine {
    const char *name; size_t size; ComponentCleanup func;
}ComponentDefine; 
// End Array with {0, 0} to signify end of component definitions

/* Complex Arrays */
typedef struct BADef *BA;

typedef struct BADefine {
    uint32_t datasize;
    uint32_t datacount;
} BADefine;

/* Functions */

nodisc ECS_t  ECS_Create( size_t entitycount, ComponentDefine *components );
void          ECS_Destroy( ECS_t ecs );
nodisc Comp_t ECS_GetComp( ECS_t ecs, const char *name );

nodisc Entity_t ECS_AddEntity( ECS_t ecs );
void            ECS_RemoveEntity( ECS_t ecs, Entity_t entity );
nodisc bool     ECS_Exists( ECS_t ecs, Entity_t entity );
void            ECS_AddComp( ECS_t ecs, Entity_t entity, Comp_t comp, Data_t data );
void            ECS_RemoveComp( ECS_t ecs, Entity_t entity, Comp_t comp );
nodisc bool     ECS_Has( ECS_t ecs, Entity_t entity, Comp_t comp ); 
nodisc Data_t   ECS_Get( ECS_t ecs, Entity_t entity, Comp_t comp );
nodisc Data_t   ECS_Get0( ECS_t ecs, Comp_t comp ); // Gets the pointer to the start of the component array, even if entity 0 doesnt have component `comp`.

nodisc BA BACreate( BADefine def );
void   BADestroy( BA comarr );

nodisc uint32_t BAGetStride( BA comarr );
void            BASet( BA comarr, Data_t data, uint32_t count, uint32_t at );
nodisc Data_t   BaGetPointer( BA comarr, uint32_t at );
             

#endif
