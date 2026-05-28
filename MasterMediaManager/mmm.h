#if !defined(MMM_HEADER_INCLUDED)
#define      MMM_HEADER_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined( GRAPHICS_STRUCTS )
#define        GRAPHICS_STRUCTS

typedef struct Colour {
    unsigned char r, g, b, a;
} Colour;
#define Colours_white (Colour){255, 255, 255, 255}
#define Colours_black (Colour){000, 000, 000, 255}
#define Colours_red   (Colour){255, 000, 000, 255}
#define Colours_green (Colour){000, 255, 000, 255}
#define Colours_blue  (Colour){000, 000, 255, 255}
#define Colours_      (Colour){000, 000, 000, 000}

typedef struct DirectImage {
    Colour *data;
    uint32_t width, height;
} DirectImage;

#endif /* GRAPHICS_STRUCTS */

/* direct images */
[[__nodiscard__]] DirectImage directimage_create_bmp(const char *file);
void                          directimage_destroy(DirectImage *di);



#if defined(MMM_IMPLEMENTATION)
DirectImage directimage_create_bmp(const char *file){
    DirectImage ret;
    size_t filesize;
    void  *filedata;

    {
        FILE *fileread = fopen(file, "rb");

        fseek(fileread, 0, SEEK_END);
        filesize = ftell(fileread);
        fseek(fileread, 0, SEEK_SET);

        filedata = malloc(sizeof(char) * filesize);
        fread(filedata, 1, filesize, fileread);
        fclose(fileread);
    }

    uint32_t offset = *(uint32_t*)(filedata + 10);

    uint32_t sizeofDIB = *(uint32_t*)(filedata+14);

    if (sizeofDIB != 56){
        return (DirectImage){
            0, 0, 0
        };
    }

    /* THIS CODE ASSUMES THE BMP HAS HEADER BITMAPV3INFOHEADER WITH:
        - A B8-G8-R8-A8 PIXEL FORMAT
        - WITH NO ENCRYPTION 
        - PIXELS SOTRED LEFT-RIGHT, BOTTOM-TOP
    */

    ret.width = *(int32_t*)(filedata+18);
    ret.height = *(int32_t*)(filedata+22);

    ret.data = malloc(ret.width * ret.height * 4);
    memcpy( ret.data, (filedata+offset), ret.width * ret.height * 4 );

    /* SWAP THE BOTTOM-TOP FORMAT TO TOP-BOTTOM */
    Colour *swap = malloc(ret.width * 4);
    for (uint32_t i = 0; i < (ret.height / 2); ++i){
        uint32_t top, bottom;
        top = ret.width * i; bottom = ret.width * (ret.height - 1 - i);
        memcpy( swap, ret.data + top, ret.width * 4 );
        memcpy( ret.data + top, ret.data + bottom, ret.width * 4 );
        memcpy( ret.data + bottom, swap, ret.width * 4 );
     // printf("top %d, bottom %d\n", top, bottom);
    }
    free( swap );

    /* CHANGE B8-G8-R8-A8 PIXEL FORMAT TO R8-G8-B8-A8 */
    for(uint32_t i = 0; i < ret.width * ret.height; ++i){
        Colour cpy = ret.data[i];
        ret.data[i] = (Colour){cpy.b, cpy.g, cpy.r, cpy.a};
    }

    return ret;
}
void directimage_destroy(DirectImage *di){
    free(di->data);
    di->width = 0;
    di->height = 0;
}
#endif /* MMM_IMPLEMENTATION */

#endif /* MMM_HEADER_INCLUDED */
