#include "zubwayengine.h"
#include "ui.h"
#include "geometry.h"

#include <stdio.h>
#include <stdlib.h>

uint32_t AlignTo( uint32_t num, uint32_t alignment ){
    return num + (alignment - (num % alignment));
}

ECS_t ecs;

typedef struct ObjVertex{
    Vector3 pos;
    Vector3 nrm;
    Vector2 tex;

    uint32_t trsid;
    uint32_t matid;
} ObjVertex;
typedef struct UiVertex {
    Vector2 pos, tex;
    Vector4 col;
} UiVertex;
typedef struct LandVertex {
    Vector3 pos;
    Vector3 nrm;
    uint32_t x;
} LandVertex;

struct LandPushConst{
    Matrix cam;
    Vector3 sunray;
    Vector2 mouse;
};

struct {
    float  width, depth;
    float *height;
    Vector3 *norms;
    float scale;

    Drawable land;
} WorldData;

Drawer landdrawer, objdrawer, uidrawer;

void CreateDrawers( void ){ 
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
        .constdatasize = 64,
        .drawmethod = DDME_triangle,
        .transparency = true,

        .vertexcount = 50000,
        .indexcount = 1000000,

        .is2d = false
    };
    objdrawer = DrawerCreate( dci );
    
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
        .constdatasize = 0,
        .drawmethod = DDME_triangle,
        .transparency = true,

        .vertexcount = 1000,
        .indexcount = 100,

        .is2d = true
    };
    uidrawer = DrawerCreate( dci2 );

    DrawerVertexInfo dvi3[] = {
        // binding, location, data type
        {0, 0, DDE_float3},
        {0, 1, DDE_float3},
        {0, 2, DDE_uint1},
    };
    DrawerResourceInfo dri3[] = { 
    };
    DrawerCreateInfo dci3 = {
        .vertexinfos = dvi3,
        .vertexinfocount = 3,
        .resourceinfos = dri3,
        .resourceinfocount = 0,

        .vshader = "land.vert.spirv",
        .fshader = "land.frag.spirv",
        .constdatasize = AlignTo(sizeof(struct LandPushConst), 8),
        .drawmethod = DDME_triangle,
        .transparency = true,

        .vertexcount = 50000,
        .indexcount = 1000000,

        .is2d = false
    };
    landdrawer = DrawerCreate( dci3 );

};

void GridFill( MeshFillData data ){
    ObjVertex v = {
        {(data.grid.x * 10) - 5.0, (data.grid.y * 10) - 5.0, 0},
        {0, 0, 0},
        {data.grid.x, data.grid.y},
        ((uint32_t*)data.exdata)[0],
        ((uint32_t*)data.exdata)[1]
    };
    *(ObjVertex*)data.toptr = v;
}

void PyrFill( MeshFillData data ){
    ObjVertex v = {
        {data.spyramid.x - 0.5, 0.0 - data.spyramid.y, data.spyramid.z - 0.5},
        {0, 0, 0},
        {data.spyramid.x * 0.25, data.spyramid.z * 0.25},
        ((uint32_t*)data.exdata)[0],
        ((uint32_t*)data.exdata)[1]
    };
    *(ObjVertex*)data.toptr = v;
};

void LandGridFill( MeshFillData data ){
    float theheight;
    uint32_t gx = data.grid.gx, gy = data.grid.gy;
    float halfscale = WorldData.scale / 2.0;

    /*
    if (gx == 0 || gy == 0){
        LandVertex v = {
            {(data.grid.x * WorldData.scale) - halfscale, 1.0, (data.grid.y * WorldData.scale) - halfscale},
            0
        };
        *(LandVertex*)data.toptr = v;   
        return;
    }
    if (gx) --gx;
    if (gy) --gy;
    */ 

    theheight = WorldData.height[
        gy * 64 + gx
    ];

    float
        ydiffz = 0,
        ydiffx = 0
    ;

    // Calculating left-right difference
    if (!gx){
        ydiffx = WorldData.height[(gy * 64) + gx + 1];
    }
    else {
        ydiffx = WorldData.height[(gy * 64) + gx - 1] - WorldData.height[(gy * 64) + gx + 1];
    }

    // Calculating top-bottom difference
    if (!gy){
        ydiffz = WorldData.height[(gy * 64) + gx + 1];
    }
    else {
        ydiffz = WorldData.height[(gy * 64) + gx - 1] - WorldData.height[(gy * 64) + gx + 1];
    }
    // ...

    Vector3 norm = Vector3CrossProduct(
        Vector3Normalize((Vector3){0, ydiffz, 1}),
        Vector3Normalize((Vector3){1, ydiffx, 0})
    );
    LandVertex v = {
        {(data.grid.x * WorldData.scale) - halfscale, 1.0 - theheight, (data.grid.y * WorldData.scale) - halfscale},
        norm,
        0
    };
    *(LandVertex*)data.toptr = v;   
}

void WorldCreate(){
    DirectImage img = directimage_create_bmp("HeightMap2.bmp");

    uint32_t size = img.width * img.height;

    WorldData.width = img.width;
    WorldData.depth = img.height;

    WorldData.height = malloc(sizeof(float) * img.width * img.height);
    for (uint32_t i = 0; i < size; ++i){
        WorldData.height[i] = (float)img.data[i].r / 256.0;
    }

    for (uint32_t y = 0; y < WorldData.depth; ++y){
        for (uint32_t x = 0; x < WorldData.width; ++x){
            uint32_t i = (y * WorldData.depth) + x;

            float h = WorldData.height[i];

            float rad = atanf( 1.0f / h );
        }
    }

    WorldData.scale = 15;

    directimage_destroy( &img );   

    MeshCreateInfo mci = {
        LandGridFill, 0, sizeof(LandVertex)
    };
    Grid2D g = {
        1, 1,
        8, 8,
        0, 0
    };
    Box2D b = Grid2DGetBox2D(g, 7, 1);

    DrawableCreateInfo mapregister = {
        MeshCreateGrid( 62, 62, 1.0f, b, mci ),
        true, MatrixZero()
    };
    WorldData.land  = CreateDrawable( landdrawer, mapregister );
}

Vector3 GetPosOnLand( Vector2 inpos, CameraInfo ci ){
    bool found = false;

    Vector2 mpos = WindowGetNormPos();
    mpos = Vector2Scale(mpos, 2.0);
    Vector3 from = ci.position;

    Vector3 raystep = CameraGetRay( ci, mpos ); 

    raystep = Vector3Scale(raystep, 0.01);

    uint32_t intoloop = 0;
    while (!found) {
        if (intoloop++ > 10000) break;

        from = Vector3Add( from, raystep );

        //from.x += 1.0 / (float)WorldData.scale;
        //from.z += 1.0 / (float)WorldData.scale;

        if (
            (fabs(from.x) > (WorldData.scale / 2.0)) ||
            (fabs(from.z) > (WorldData.scale / 2.0))
        ){
            // OUT
            if ( from.y > 1.0 ) found = true;
        }        
        else {
            // IN

            float fx = (from.x + (WorldData.scale / 2.0)) * (WorldData.width / WorldData.scale); 
            float fz = (from.z + (WorldData.scale / 2.0)) * (WorldData.depth / WorldData.scale); 

            uint32_t x = (uint32_t)roundf(fx);
            uint32_t z = (uint32_t)roundf(fz);

            float h = WorldData.height[ x + (z * (uint32_t)WorldData.width) ];
            if ( from.y > 1.0 - h ){
                found = true;
            }

        }

    }
    if (found){
        //printf("yaaaay %f %f %f\n", from.x, from.y, from.z);
        return from;
    }

    //printf("%f %f %f\n", raystep.x, raystep.y, raystep.z);

    return (Vector3){0, 0, 0};
}



int main(){
    InitWindow( 1280, 720, "HIYA", 0 );

    CreateDrawers();

    WorldCreate();

    /* ECS example
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
    */
 
    Grid2D g = {
        1, 1,
        8, 8,
        0, 0
    };
    Box2D b = Grid2DGetBox2D(g, 7, 1);
    uint32_t trsmat[2] = {0, 0};
    uint32_t trsmad[2] = {1, 1};

    MeshCreateInfo mci = {
        GridFill, trsmat, sizeof(ObjVertex)
    };
    MeshCreateInfo mci2 = {
        PyrFill, trsmad, sizeof(ObjVertex)
    };

    DrawableCreateInfo pyrreg = {
        MeshCreateSPyramid( 1.0, b, mci2 ),
        true, MatrixScale(0.1, 0.1, 0.1)//MatrixZero()
    };
    Drawable pyrdrawable  = CreateDrawable( objdrawer, pyrreg );

    ExitOnError(DrawableSetTransform(
        pyrdrawable, MatrixIdentity()
    ));

    UiVertex testdata[] = {
        {{-0.01,  0.01}, {0,0}, {0,0,0,0}},
        {{-0.01, -0.01}, {0,0}, {0,0,0,0}},
        {{ 0.01,  0.01}, {0,0}, {0,0,0,0}},
    };
    uint32_t testdata2[] = {0, 1, 2};
    MeshResource_t testmesh = {
        .vertdata = testdata, .vertcount = 3,
        .inddata = testdata2, .indcount = 3,
        .vertsize = sizeof(UiVertex),
        0, 0
    };
    DrawableCreateInfo testreg = {
        testmesh, false,
        MatrixZero()
    };
    Drawable testdrawable = CreateDrawable( uidrawer, testreg );

    CameraInfo ci = {
        (Vector3){0, 0, 1},
        0, 0,
        3.14159 / 3.0, 1280.0 / 720.0
    };
    camera_set_main_camera( ci );

    Vector3 geebin = {0,0,0};

    float timeofday = 0;
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
        geebin = GetPosOnLand( (Vector2){0, 0}, ci );
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
        
        ExitOnError(DrawableSetTransform(
            pyrdrawable, MatrixTranslate(geebin.x, geebin.y, geebin.z)
        ));
        
        WindowStartDrawing();
        
        Matrix m = MatrixMultiply(
            CameraGetView( ci ),
            CameraGetProj( ci )
        );

        Vector2 mousep = {0, 0};
        
        Vector3 sunray = {0, 1, 0};
        sunray = Vector3Transform(
            sunray,
            MatrixRotateZ( timeofday )
        );
        
        // ENABLE FOR DAY/NIGHT CYCLE
        //timeofday += 3.14159 * (0.5 / 60.0);

        timeofday = 1;

        struct LandPushConst lpc = {
            m,
            sunray,
            mousep
        };

        WindowDraw( landdrawer, &lpc );
        WindowDraw( objdrawer, &m );
        WindowDraw( uidrawer, 0 );
        
        WindowFinishDrawing();
    }

    return 0;
}

