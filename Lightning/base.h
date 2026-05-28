#if !defined(__BASE_H)
#define      __BASE_H

#include <stddef.h>
#include <stdint.h>

extern int printf(const char *, ...);
extern void exit(int32_t);

/* FOR DEFINITIONS */
#define nodisc [[__nodiscard__]]
typedef const char *Return_t; 

/* SYSTEM STUFF */
#define ezd() printf("got to line %d", __LINE__); // easy debug

void ExitOnError( Return_t r );

#if defined( __BASE_IMPLEMENTATION )
void ExitOnError( Return_t r ){
    if (r){
        printf("%s\n", r);
        exit(-1);
    }
}
#endif

#endif