#include "zubwayengine.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>

ECS_t ecs;

typedef struct Position{
    float x, y, z;
} Position;

int main(){
    InitWindow( 1280, 720, "HIYA", 0 );

    DrawerVertexInfo dvi[] = {
        {0, 0, },
    };
    DrawerResourceInfo dri[] = {
        
    };
    DrawerCreateInfo dci = {
        .vertexinfos = dvi,
        .vertexinfocount = 4,
        .resourceinfos = dri,
        .resourceinfocount = 2,

        .vshader = "world.vert.spirv",
        .fshader = "world.frag.spirv",
        .drawmethod = DDME_triangle
    };
    printf("balls\n");
    Drawer dr = DrawerCreate( dci );
    printf("cum\n");

    

    ComponentDefine comps[] = {
        {"pos", sizeof(Position), 0},
        {"gfx", sizeof(Drawable), 0},
        {0, 0, 0}
    };
    ecs = ECS_Create( 1024, comps );
    Comp_t poscomp = ECS_GetComp(ecs, "pos");

    Entity_t mapent = ECS_AddEntity( ecs );
    Vector3 mappos = {
        0, 0, 0
    };
    ECS_AddComp(ecs, mapent, poscomp, &mappos);
    ECS_AddComp(ecs, mapent, ECS_GetComp(ecs, "gfx"), 0);

    RegisterDrawableInfo mapregister = {
        Mesh_CreateQuad( MatrixIdentity(), 0, 0 ), true,
        MatrixRotateX(3.14159 / 2.0)
    };
    Drawable mapdrawable = CreateDrawable( mapregister );
    ExitOnError(DrawableSetTransform( mapdrawable, MatrixIdentity() ));

    CameraInfo ci = {
        (Vector3){0, -1, 1},
        -PI/3.0, 0,
        3.14159 / 3.0, 1280.0 / 720.0
    };
    camera_set_main_camera( ci );
 
    UiComponent root = ui_create_root();
    UiComponent cmon = ui_create_text( root, "HELLO UI", 12);

    ui_generate( root );

    while (AppRunning()){
        WindowClearScreen( );
        WindowEvent *events = GetWindowEvents(), *e = events;
        while (1){
            if (e->type == WET_Start){}
            else if (e->type == WET_End) break;
            else if (e->type == WET_Key){
                if (e->val.key.key == 'q'){
                    TerminateApp();
                }
                /*
                else if (e->val.key.key == 'd'){
                    ci.position.x += 1.0 / 60.0;
                }
                else if (e->val.key.key == 'a'){
                    ci.position.x -= 1.0 / 60.0;
                }
                */
            }
            else if (e->type == WET_MouseClk){
            }
            else if (e->type == WET_MouseMove){
            }
            else if (e->type == WET_Geometry){
                //printf("%f %f | %f %f\n", e->val.geo.x, e->val.geo.y, e->val.geo.width, e->val.geo.height);
            }            
            else {
                printf("You're missing out on data !\n");
            }
            
            e = e->next;
        }

        if(
            window_key_down('e') &&
            window_key_down('v') &&
            window_key_down('a')
        )
        {
            printf("Eva is the most gorgeous woman ever, and I'm the luckies man EVER. I love you :)\n");
        }

        Vector3 cammoving = camera_get_forward_XZ(ci);

        if(window_key_down('w')){
            ci.position.z += cammoving.z / 60.0;
            ci.position.x += cammoving.x / 60.0;
        }
        if(window_key_down('s')){
            ci.position.z -= cammoving.z / 60.0;
            ci.position.x -= cammoving.x / 60.0;
        }

        if(window_key_down('a')){
            ci.position.z -= cammoving.x / 60.0;
            ci.position.x += cammoving.z / 60.0;
        }
        if(window_key_down('d')){
            ci.position.z += cammoving.x / 60.0;
            ci.position.x -= cammoving.z / 60.0;
        }

        if(window_key_down('j'))
            ci.yaw -= 1.0 / 60.0;
        if(window_key_down('l'))
            ci.yaw += 1.0 / 60.0;
        if(window_key_down('i'))
            ci.pitch += 1.0 / 60.0;
        if(window_key_down('k'))
            ci.pitch -= 1.0 / 60.0;

        camera_set_main_camera( ci );

        ClearWindowEvents(events);

        printf("yeah okayyyyyyy\n");
        WindowDraw( dr, mapdrawable );
        printf("never no more :(\n");
        ui_draw( root );
    }

    ui_destroy( root );

    return 0;
}

