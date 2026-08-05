#if !defined(__WINGFX_H)
#define      __WINGFX_H

#include <stdint.h>

#include "base.h"
#include "mmm.h"
#include "maths.h"
#include "logic.h"
#include "geometry.h"

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
Vector2 WindowDimensions( void );

/* GRAPHICS */


/*
DRAWABLE STRUCT
---------------

These represent the data to be passed and processed by a Drawer.
*/
typedef struct DrawableDef *Drawable;
typedef struct DrawerDef *Drawer;
struct CameraInfo;

typedef struct Vertex{
    Vector3 pos;
    Vector3 nrm;
    Vector2 tex;

    uint32_t trsid;
    uint32_t matid;
} Vertex;
typedef struct MeshResource_t {
    void     *vertdata; uint32_t vertcount;
    uint32_t *inddata;  uint32_t indcount;
    uint32_t  vertsize;
    uint32_t trspos, matpos;
} MeshResource_t;
typedef struct DrawableCreateInfo{
    MeshResource_t mesh; bool discard;
    Matrix transform;
} DrawableCreateInfo;

Drawable CreateDrawable( Drawer drawer, DrawableCreateInfo rdi );
Return_t DrawerUpdateResource( Drawer dr, uint32_t resid, Data_t data, uint32_t size, uint32_t offset );
Return_t DrawableSetVisability( Drawable dr, bool vis );
Return_t DrawableSetTransform( Drawable dr, Matrix m );

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

    bool is2d;
    struct CameraInfo *cam;
} DrawerCreateInfo;
Drawer DrawerCreate( DrawerCreateInfo dci );

uint32_t DataTypeToSize( DrawerDataEnum dde );

void WindowClearScreen( );
void WindowStartDrawing( );
void WindowDraw( Drawer drawer );
void WindowFinishDrawing( );

typedef struct CameraInfo {
    Vector3 position;
    float pitch, yaw;
    float fov, aspect;
    bool is2d;
} CameraInfo;

void camera_set_main_camera( CameraInfo cam );
CameraInfo camera_get_main_camera( );
Vector3 camera_get_forward_XZ( CameraInfo cam );
Vector3 camera_get_forward_XYZ( CameraInfo cam );

typedef struct MeshFillData {
    void *exdata;
    void *toptr;
    union {
        struct {
            float x, y;
        } grid;
    };
} MeshFillData;
typedef void (*MeshCreateFunc)(MeshFillData data);
typedef struct MeshCreateInfo{
    MeshCreateFunc func;
    void *data;
    uint32_t vertsize;
} MeshCreateInfo;

MeshResource_t MeshCreateGrid( uint32_t xdiv, uint32_t ydiv, float ratio, Box2D tex, MeshCreateInfo mci );

#endif
