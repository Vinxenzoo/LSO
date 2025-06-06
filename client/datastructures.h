#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <pthread.h>


#define MAXREADER 512
#define MAXWRITER 16

enum player_type
{
    HOST,
    OPPONENT
};

extern char grid[3][3];
extern int sd;

extern const char NOERROR;
extern const char ERROR;

extern const char NONE;
extern const char WIN;
extern const char LOSE;
extern const char DRAW;

#endif