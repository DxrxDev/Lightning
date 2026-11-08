#include "ui.h"

/*
 * THIS CODE IS VERY MUCH SUBJECT TO CHANGE,
 * THIS IS JUST A QUICK AND DIRTY PROTOTYPE
 * ONLY SUPPORTING BASIC, STATIC, UNSTYLED
 * COLOURFUL PANELS.
 * */

void UiGridFill( MeshFillData data ){
    float theheight;
    uint32_t gx = data.grid.gx, gy = data.grid.gy;


    UiComponentInfo comp = *(UiComponentInfo*)data.exdata;

    float x = comp.panel.x + (comp.panel.w * data.grid.x);
    float y = comp.panel.y + (comp.panel.h * data.grid.y);

    UiVertex v = {
        {x, y},
        {0, 0},
        comp.panel.colour
    };
    *(UiVertex*)data.toptr = v;   
}

typedef struct ElementBasics {
    uint32_t x, y;

    Drawable dr;
} ElementBasics;

struct UiComponent_def {

};

struct UiManager_def {
    Drawer dr;
    ECS_t elements;
    
};

UiManager UiCreate( float x, float y, float w, float h ){
    UiManager ret = malloc( sizeof(struct UiManager_def) );

    DrawerVertexInfo dvi[] = {
        {0, 0, DDE_float2},
        {0, 1, DDE_float2},
        {0, 2, DDE_float4}
    };
    DrawerResourceInfo dri[] = { 
        {
            0, 0,
            DRTE_sampler, DRSE_fragment,
            .sampler = {
                "texture_map.bmp"
            }
        },
    };
    DrawerCreateInfo dci = {
        .vertexinfos = dvi,
        .vertexinfocount = 3,
        .resourceinfos = dri,
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
    ret->dr = DrawerCreate( dci );
    
    ComponentDefine elementcomponents[] = {
        { "basics", sizeof( ElementBasics ), NULL },
        { 0, 0, NULL }
    };

    ret->elements = ECS_Create( 1024, elementcomponents );

    return ret;
}

Drawer UiGetDrawer( UiManager uim ){
    return (uim) ? uim->dr : 0;
}

void UiRedraw( UiManager man ){
    return;
}

uint32_t UiAddComponent( UiManager man, UiComponent parent, UiComponentInfo uci ){
    if (!man) return UINT32_MAX;

    Entity_t added = ECS_AddEntity( man->elements );

    MeshCreateInfo mci = {
        UiGridFill, &uci, sizeof(UiVertex)
    };
    Grid2D g = {
        1, 1,
        8, 8,
        0, 0
    };
    Box2D b = Grid2DGetBox2D(g, 0, 0);

    DrawableCreateInfo reg = {
        MeshCreateGrid( 0, 0, 1.0f, b, mci ),
        true, MatrixZero()
    };
    Drawable drawable  = CreateDrawable( man->dr, reg );

    ElementBasics eb = {0, 0, drawable};
    ECS_AddComp( man->elements, added, 0, &eb );

    return added;
}
