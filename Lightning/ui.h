
/*
#if !defined( SOUL_UI_HEADER )
#define       SOUL_UI_HEADER

#define UI_CENTRED UINT32_MAX
#define UI_DEFAULT_FONT 0

typedef struct UiDismension {
    uint32_t x, y, onx, ony; 
} UiDimensions;

typedef struct UiBase {
    UiDimensions dims;
} UiRoot;

typedef struct UiLabel {
    UiRoot root;
    const char *text;
} UiLabel;

UiBase ui_root_create( UiDimensions dims );
UiLabel ui_label_create( UiDimensions dims, const char * );

uint32_t ui_get_text_width( const char *text );

#endif  SOUL_UI_HEADER */
