#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <pthread.h>

//dimensioni buffer per leggere e scrivere sulla socket
#define MAXREADER 512
#define MAXWRITER 16

//serve alla funzione partita per gestire gli input
enum player_type
{
    HOST,
    OPPONENT
};

extern char grid[3][3];
extern int sd;

//variabili di errore ricevute dal server
extern const char NOERROR;
extern const char ERROR;

//costanti esito partita
extern const char NONE;
extern const char WIN;
extern const char LOSE;
extern const char DRAW;

#endif