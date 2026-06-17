#if !defined(__GEOMETRY_H)
#define      __GEOMETRY_H

#include "maths.h"
#include "stdint.h"

/* Geometries */

typedef struct Box2D {
    float x, y, w, h;
} Box2D;
typedef struct Grid2D {
    float width, height;
    uint32_t xdiv, ydiv;
    float offsetx, offsety;
} Grid2D;

/* Functions */

Vector2 Box2DGetCentre( Box2D box );
Vector2 Box2DGetCorner( Box2D box, uint32_t corner ); // TL, TR, BL, BR
                                                      // 0,  1,  2,  3

Box2D Grid2DGetBox2D( Grid2D grid, uint32_t xat, uint32_t yat );


#endif // __GEOMETRY_H
