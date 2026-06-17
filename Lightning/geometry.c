#include "geometry.h"

Vector2 Box2DGetCentre( Box2D box ){
    return (Vector2){
        box.x + (box.w / 2.0),
        box.y + (box.h / 2.0),
    };
}

Vector2 Box2DGetCorner( Box2D box, uint32_t corner ){
    switch (corner){
    case 0:
        return (Vector2){box.x, box.y};
    case 1:
        return (Vector2){box.x + box.w, box.y};
    case 2:
        return (Vector2){box.x, box.y + box.h};
    case 3:
        return (Vector2){box.x + box.w, box.y + box.h};
    };
}

Box2D Grid2DGetBox2D( Grid2D grid, uint32_t xat, uint32_t yat ){
    float cw = grid.width / (float)grid.xdiv;
    float ch = grid.height / (float)grid.ydiv;
    return (Box2D){
        cw * xat, ch * yat,
        cw, ch
    };
}
