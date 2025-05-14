#ifndef DATASTRUCT_H
#define DATASTRUCT_H

//Used libraries
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>
#include <stdbool.h>

#define  MAX_NAME_PLAYER 20
#define  MESSAGE_SIZE 64

enum game_status
{
  NEW_GAME,
  WAITING,
  RUNNING,
  END_GAME
};

enum game_result
{
  NONE,
  VICTORY,
  DEFEAT,
  DRAW
};

enum player_status
{
  IN_LOBBY,
  REQUESTING,
  IN_GAME
};

typedef struct ServerStruct //Pascal Case
{
    int fd;
    socklen_t addrlen;
    struct sockaddr_in address;
    unsigned short int opt;

}Server; //Pascal Case


struct PlayerNode
{
  char name[MAX_NAME_PLAYER];
  enum player_status status;
  pthread_cond_t cv_state;
  pthread_mutex_t mutex_state;
  bool champion;
  unsigned int wins;
  unsigned int losts;
  unsigned int draws;
  pthread_t player_id;
  int player_sd;
  struct PlayerNode *next_node;
}players;


struct GameNode
{
  char owner[MAX_NAME_PLAYER];
  int owner_sd;
  char enemy[MAX_NAME_PLAYER];
  int enemy_sd;
  enum game_status status;
  bool join_request;
  pthread_cond_t cv_state;
  pthread_mutex_t mutex_state;
  struct GameNode *next_node;
}games;


#endif
