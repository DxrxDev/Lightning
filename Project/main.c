#include "zubwayengine.h"
#include "ui.h"
#include "geometry.h"

#include <stdio.h>
#include <stdlib.h>

ECS_t ecs;

typedef struct Position{
    float x, y, z;
} Position;

typedef struct {
    Vector2 pos, tex;
    Vector4 col;
} Vert2D;

struct {
    float  width, depth;
    float *height;
} WorldData;

void GridFill( MeshFillData data ){
    Vertex v = {
        {(data.grid.x * 10) - 5.0, (data.grid.y * 10) - 5.0, 0},
        {0, 0, 0},
        {data.grid.x, data.grid.y},
        ((uint32_t*)data.exdata)[0],
        ((uint32_t*)data.exdata)[1]
    };
    *(Vertex*)data.toptr = v;
}

void PyrFill( MeshFillData data ){
    Vertex v = {
        {data.spyramid.x - 0.5, 1.0 - data.spyramid.y, data.spyramid.z - 0.5},
        {0, 0, 0},
        {data.spyramid.x * 0.25, data.spyramid.z * 0.25},
        ((uint32_t*)data.exdata)[0],
        ((uint32_t*)data.exdata)[1]
    };
    *(Vertex*)data.toptr = v;
};

void ApplyHeightMap( MeshResource_t *mesh ){
    DirectImage img = directimage_create_bmp("HeightMap2.bmp");

    uint32_t size = img.width * img.height;

    for (uint32_t i = 0; i < size; ++i){
        Vertex *mod = mesh->vertdata;
        mod[i].pos.z += (img.data[i].r / 256.0);

    }

    directimage_destroy( &img );
}

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
                true, 1024 * sizeof( Matrix )
            }
        },
        {
            0, 1,
            DRTE_sampler, DRSE_fragment,
            .sampler = {
                "HeightMap2.bmp"
            }
        },
        {
            0, 2,
            DRTE_sampler, DRSE_fragment,
            .sampler = {
                "Buildings.bmp"
            }
        },
    };
    DrawerCreateInfo dci = {
        .vertexinfos = dvi,
        .vertexinfocount = 5,
        .resourceinfos = dri,
        .resourceinfocount = 3,

        .vshader = "world.vert.spirv",
        .fshader = "world.frag.spirv",
        .drawmethod = DDME_triangle,
        .transparency = true,

        .vertexcount = 50000,
        .indexcount = 1000000,

        .is2d = false
    };
    Drawer worlddrawer = DrawerCreate( dci );
    
    DrawerVertexInfo dvi2[] = {
        {0, 0, DDE_float2},
        {0, 1, DDE_float2},
        {0, 2, DDE_float4}
    };
    DrawerResourceInfo dri2[] = { 
        {
            0, 0,
            DRTE_sampler, DRSE_fragment,
            .sampler = {
                "texture_map.bmp"
            }
        },
    };
    DrawerCreateInfo dci2 = {
        .vertexinfos = dvi2,
        .vertexinfocount = 3,
        .resourceinfos = dri2,
        .resourceinfocount = 1,

        .vshader = "ui.vert.spirv",
        .fshader = "ui.frag.spirv",
        .drawmethod = DDME_triangle,
        .transparency = true,

        .vertexcount = 1000,
        .indexcount = 100,

        .is2d = true
    };
    Drawer uidrawer = DrawerCreate( dci2 );

    printf("we here?\n");

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
    Box2D b = Grid2DGetBox2D(g, 7, 1);
    uint32_t trsmat[2] = {0, 0};
    uint32_t trsmad[2] = {1, 1};

    MeshCreateInfo mci = {
        GridFill, trsmat, sizeof(Vertex)
    };
    MeshCreateInfo mci2 = {
        PyrFill, trsmad, sizeof(Vertex)
    };

    //MeshResource_t mesh = MeshCreateGrid( 30, 30, 1.0f, b, mci );
    //free(mesh.vertdata);
    //free(mesh.inddata);

    DrawableCreateInfo mapregister = {
        MeshCreateGrid( 62, 62, 1.0f, b, mci ),
        true, MatrixZero()
    };
    ApplyHeightMap( &mapregister.mesh );
    Drawable mapdrawable  = CreateDrawable( worlddrawer, mapregister );

    DrawableCreateInfo pyrreg = {
        MeshCreateSPyramid( 1.0, b, mci2 ),
        true, MatrixZero()
    };
    Drawable pyrdrawable  = CreateDrawable( worlddrawer, pyrreg );

    ExitOnError(DrawableSetTransform(
        mapdrawable,
        MatrixMultiply(
            MatrixRotateX(3.14159 / 2.0),
            MatrixTranslate( 0, 1, 0 )
        )
    ));
    ExitOnError(DrawableSetTransform(
        pyrdrawable, MatrixRotateY( 3.14159 )//MatrixIdentity()
    ));


    Vert2D testdata[] = {
        {{-0.01,  0.01}, {0,0}, {0,0,0,0}},
        {{-0.01, -0.01}, {0,0}, {0,0,0,0}},
        {{ 0.01,  0.01}, {0,0}, {0,0,0,0}},
    };
    uint32_t testdata2[] = {0, 1, 2};
    MeshResource_t testmesh = {
        .vertdata = testdata, .vertcount = 3,
        .inddata = testdata2, .indcount = 3,
        .vertsize = sizeof(Vert2D),
        0, 0
    };
    DrawableCreateInfo testreg = {
        testmesh, false,
        MatrixZero()
    };
    Drawable testdrawable = CreateDrawable( uidrawer, testreg );


//    CameraInfo ci = {
//        (Vector3){0, -1, 1},
//        -PI/3.0, 0,
//        3.14159 / 3.0, 1280.0 / 720.0
//    };
    CameraInfo ci = {
        (Vector3){0, 0, 1},
        0, 0,
        3.14159 / 3.0, 1280.0 / 720.0
    };
    camera_set_main_camera( ci );


    while (AppRunning()){
        //WindowClearScreen( );
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

        //if (window_key_down('t'))
        //if (window_key_down('g'))

        camera_set_main_camera( ci );

        ClearWindowEvents(events);

        WindowStartDrawing();

        WindowDraw( worlddrawer );
        WindowDraw( uidrawer );
        
        WindowFinishDrawing();
    }

    return 0;
}

