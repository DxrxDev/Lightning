#include "zubwayengine.h"
#include "ui.h"
#include "geometry.h"

#include <stdio.h>
#include <stdlib.h>

ECS_t ecs;

typedef struct Position{
    float x, y, z;
} Position;

int main(){
    InitWindow( 1280, 720, "HIYA", 0 );

    DrawerVertexInfo dvi[] = {
        // binding, location, data type
        {0, 0, DDE_float3},
        {0, 1, DDE_float3},
        {0, 2, DDE_float2},
        {0, 3, DDE_uint1},
        {0, 4, DDE_uint1},
    };
    DrawerResourceInfo dri[] = {
        {
            0, 0, 
            DRTE_uniform, DRSE_vertex,
            .uniform = {
                true, 1024
            }
        },
        {
            0, 1,
            DRTE_sampler, DRSE_fragment,
            .sampler = {
                "texture_map.bmp"
            }
        },
    };
    DrawerCreateInfo dci = {
        .vertexinfos = dvi,
        .vertexinfocount = 5,
        .resourceinfos = dri,
        .resourceinfocount = 2,

        .vshader = "world.vert.spirv",
        .fshader = "world.frag.spirv",
        .drawmethod = DDME_triangle,
        .transparency = true
    };
    Drawer worlddrawer = DrawerCreate( dci );

    /*
    DrawerVertexInfo dvi2[] = {
        {0, 0, DDE_float2},
        {0, 1, DDE_float2},
        {0, 2, DDE_float4}
    };
    DrawerResourceInfo dri2[] = { 
        {0, 0, DRTE_sampler, DRSE_fragment},
    };
    DrawerCreateInfo dci2 = {
        .vertexinfos = dvi2,
        .vertexinfocount = 3,
        .resourceinfos = dri,
        .resourceinfocount = 2,

        .vshader = "ui.vert.spirv",
        .fshader = "ui.frag.spirv",
        .drawmethod = DDME_triangle
    };
    Drawer uidrawer = DrawerCreate( dci2 );
    */

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

    Grid2D g = {
        1, 1,
        8, 8,
        0, 0
    };
    Box2D b = Grid2DGetBox2D(g, 7, 0);

    RegisterDrawableInfo mapregister = {
        Mesh_CreateQuad( MatrixIdentity(), b, 0, 0 ), true,
        MatrixRotateX(3.14159 / 2.0), 0, worlddrawer
    };
    RegisterDrawableInfo testerreg = {
        Mesh_CreateQuad( MatrixIdentity(), b, 0, 0 ), true,
        MatrixIdentity(), 0, worlddrawer
    };
    Drawable mapdrawable = CreateDrawable( mapregister );
    Drawable testdrawable = CreateDrawable( testerreg );
    ExitOnError(DrawableSetTransform( mapdrawable, MatrixIdentity() ));
    ExitOnError(DrawableSetTransform( testdrawable, MatrixIdentity() ));

    CameraInfo ci = {
        (Vector3){0, -1, 1},
        -PI/3.0, 0,
        3.14159 / 3.0, 1280.0 / 720.0
    };
    camera_set_main_camera( ci );
 
    UiComponent root = ui_create_root();
    UiComponent cmon = ui_create_text( root, "HELLO UI", 12);

    ui_generate( root );

    float balls = 0;
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

        if (window_key_down('t'))
            balls += 0.016;
        if (window_key_down('g'))
            balls -= 0.016;

        camera_set_main_camera( ci );

        ClearWindowEvents(events);
    
        
        ExitOnError(DrawableSetTransform( testdrawable, MatrixTranslate(0, balls, 0) ));

        WindowDraw( worlddrawer, mapdrawable );
        ui_draw( root );
    }

    ui_destroy( root );

    return 0;
}

