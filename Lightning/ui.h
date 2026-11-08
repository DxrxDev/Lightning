#if !defined( SOUL_UI_HEADER )
#define       SOUL_UI_HEADER

#include "wingfx.h"

#define UI_HALLIGN_LEFT (-1)
#define UI_HALLIGN_CENTRE (0)
#define UI_HALLIGN_RIGHT (1)
#define UI_VALLIGN_TOP (-1)
#define UI_VALLIGN_CENTRE (0)
#define UI_VALLIGN_BOTTOM (1)

#define UI_FILL_X (0b1)
#define UI_FILL_Y (0b10)

#define UI_BASE_SETTINGS (UiComponentSettings){0, 0, 0}

#define UI_NONE (0)
#define UI_PANEL (1)
#define UI_TYPE_ENUM_SIZE (1)

typedef struct UiVertex {
    Vector2 pos, tex;
    Vector4 col;
} UiVertex;

typedef struct UiManager_def   *UiManager;
typedef struct UiComponent_def *UiComponent;

typedef struct UiComponentSettings { 
    int8_t hallign; // -1=left, 0=centre, 1=right
    int8_t vallign; // -1=top,  0=centre, 1=bottom
    int8_t fill; // 0x1=fill on x, 0x2=fill on y
} UiComponentSettings;

typedef struct UiPanelInfo{
    float x, y, w, h;
    union {
        Vector4 color;
        Vector4 colour;
    };
} UiPanelInfo;
typedef struct UiComponentInfo {
    UiComponentSettings settings;
    uint32_t type;
    union {
        UiPanelInfo panel;
    };
} UiComponentInfo;

UiManager UiCreate( float x, float y, float w, float h );
Drawer UiGetDrawer( UiManager uim );
void UiRedraw( UiManager man );

uint32_t UiAddComponent( UiManager man, UiComponent parent, UiComponentInfo uci );
void     UiDestroyComponent( UiManager man, UiComponent parent );
UiComponent UiGet( UiManager man, UiComponent parent, uint32_t index );
UiComponentInfo UiGetInfo( UiManager man, UiComponent comp );

#endif // SOUL_UI_HEADER
