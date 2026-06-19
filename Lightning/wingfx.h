#if !defined(__WINGFX_H)
#define      __WINGFX_H

#include <stdint.h>

#include "base.h"
#include "mmm.h"
#include "maths.h"
#include "logic.h"
#include "geometry.h"

// typedef struct VkSurfaceKHR_T *VkSurfaceKHR;

/* WINDOW BASE STUFF */

enum WindowEventType {
    WET_Start = 0,
    WET_End,
    WET_Key,
    WET_MouseClk,
    WET_MouseMove,
    WET_Geometry
};

#define NUMBER_MOUSE_BUTTONS 3
enum MouseButton {
    MB_Left, MB_Middle, MB_Right
};

struct WE_Key {
    uint32_t key;
    bool pressed;
    bool shift, control;
};
struct WE_MouseClk {
    enum MouseButton mb;
    bool pressed;
};
struct WE_MouseMove {
    float x, y;
    float rootx, rooty;
};
struct WE_VisChange{
    bool visable;
};
struct WE_Geometry{
    float x, y;
    float width, height;
    // bool moved, resized;
};
typedef struct WindowEvent {
    enum WindowEventType type;
    union {
        struct WE_Key       key;
        struct WE_MouseClk  mc;
        struct WE_MouseMove mm;
        struct WE_VisChange vc;
        struct WE_Geometry  geo;
    } val;
    struct WindowEvent *next;
} WindowEvent;

enum WindowCreationFlags {
    WCF_None = 0x0,
    WCF_Resizable = 0x1
};

enum WindowError {
    WERR_success = 0
};

void InitWindow( uint32_t width, uint32_t height, const char *title, enum WindowCreationFlags flag );
[[__noreturn__]] void TerminateApp( );
void *GetWindowSystem( void );

nodisc WindowEvent *GetWindowEvents( void );
void ClearWindowEvents( WindowEvent *events );
nodisc bool AppRunning( );

bool window_key_down(char c);

Vector2 window_centre( void );

/* GRAPHICS */


/*
DRAWABLE STRUCT
---------------

These represent the data to be passed and processed by a Drawer.
*/
typedef struct DrawableDef *Drawable;
typedef struct DrawerDef *Drawer;

typedef struct Vertex{
    Vector3 pos;
    Vector3 nrm;
    Vector2 tex;

    uint32_t trsid;
    uint32_t  matid;
} Vertex;
typedef struct MeshResource_t {
    Vertex   *vertdata; uint32_t vertcount;
    uint32_t *inddata;  uint32_t indcount;
} MeshResource_t;
typedef struct DrawableCreateInfo{
    MeshResource_t mesh; bool discard;
    Matrix transform;
} DrawableCreateInfo;

Drawable CreateDrawable( Drawer drawer, DrawableCreateInfo rdi );
Return_t DrawableSetVisability( Drawable dr, bool vis );
Return_t DrawableSetTransform( Drawable dr, Matrix m );

// void UpdateVertexBuffer( MeshResource_t t, uint32_t offset );
// void UpdateIndexBuffer( MeshResource_t t, uint32_t offset );

/*
DRAWER STRUCT
-------------

These are responsible for defining how data is passed to, and processed on the GPU.
Essentially, abstractions for Vulkan's Pipelines.
*/
typedef enum DrawerDataEnum {
    DDE_float1,
    DDE_float2,
    DDE_float3,
    DDE_float4,
    DDE_uint1
} DrawerDataEnum;
typedef enum DrawerDrawMethodEnum {
    DDME_triangle
} DrawerDrawMethodEnum;
typedef enum DrawerResourceTypeEnum {
    DRTE_uniform,
    DRTE_image,
    DRTE_sampler
}DrawerResourceTypeEnum;
typedef enum DrawerResourceStageEnum {
    DRSE_vertex,
    DRSE_fragment,
    DRSE_
}DrawerResourceStageEnum;
typedef struct DrawerVertexInfo {
    uint32_t binding, location;
    DrawerDataEnum type;
} DrawerVertexInfo;
typedef struct DrawerResourceInfo {
    uint32_t set, binding;
    DrawerResourceTypeEnum type;
    DrawerResourceStageEnum stage;
    union {
        struct {
            bool hostvisable;
            uint32_t size;
        } uniform;
        struct {
            const char *file;
        } sampler;
    };
} DrawerResourceInfo;
typedef struct DrawerCreateInfo {
    DrawerVertexInfo *vertexinfos;
    uint32_t vertexinfocount;
    DrawerResourceInfo *resourceinfos;
    uint32_t resourceinfocount;

    const char *vshader;
    const char *fshader;
    DrawerDrawMethodEnum drawmethod;
    bool transparency;

    uint32_t vertexcount, indexcount;
} DrawerCreateInfo;
Drawer DrawerCreate( DrawerCreateInfo dci );

uint32_t DataTypeToSize( DrawerDataEnum dde );

void WindowClearScreen( );
void WindowDraw( Drawer drawer, Drawable drawable );

typedef struct CameraInfo
{
    Vector3 position;
    float pitch, yaw;
    float fov, aspect;
} CameraInfo;

void camera_set_main_camera( CameraInfo cam );
CameraInfo camera_get_main_camera( );
Vector3 camera_get_forward_XZ( CameraInfo cam );
Vector3 camera_get_forward_XYZ( CameraInfo cam );

#define UI_VISUAL(x) (x.type & 0x00'ff)
#define UI_TYPE_INVALID 0xff'ff
#define UI_TYPE_ROOT    0x00'00
// -----
#define UI_TYPE_HSPLIT  0x01'00
#define UI_TYPE_VSPLIT  0x02'00
#define UI_TYPE_GRID    0x03'00
// -----
#define UI_TYPE_TEXT    0x01'ff
#define UI_TYPE_SHAPE   0x02'ff
// -----
typedef struct UiComponent_def *UiComponent;
typedef void (*UiCallbackFunc)(UiComponent *, void *);

typedef struct UiChildren {
    UiComponent them;
    uint32_t num;
    uint32_t max;
} UiChildren;
typedef struct UiSpacing {
    uint32_t mt, mr, mb, ml;
    uint32_t bt, br, bb, bl;
    uint32_t st, sr, sb, sl;
} UiSpacing;

#define UI_CONFIG_SPACING 0b0000'0000'0000'0001
typedef struct UiConfigs {
    bool usingchildren;
    UiChildren children;

    bool usingspacing;
    UiSpacing spacing;
} UiConfigs;

nodisc UiComponent ui_create_root( );
nodisc UiComponent ui_create_text( UiComponent parent, const char *name, uint32_t size );
nodisc UiComponent ui_create_shape( UiComponent parent, const char *name );
void ui_destroy( UiComponent comp );

void      ui_set_configs( UiComponent comp, UiConfigs cnf );
UiConfigs ui_get_configs( UiComponent comp );

void ui_generate( UiComponent comp );
void ui_draw( UiComponent comp );

MeshResource_t Mesh_CreateQuad( Matrix m, Box2D tex, uint32_t trsid, uint32_t matid );

#endif
