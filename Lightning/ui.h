#if !defined( SOUL_UI_HEADER )
#define       SOUL_UI_HEADER

#include "wingfx.h"


typedef struct UiComponent_def *UiComponent;
typedef struct UiDrawer_def *UiDrawer;

UiComponent UiCreate( void );

Drawer UiDrawerGet( UiDrawer );

#endif  SOUL_UI_HEADER
