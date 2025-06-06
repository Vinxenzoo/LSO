#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>
#include <stdbool.h>

#define  MAX_NAME_PLAYER 20
#define  MAX_MESSAGE_SIZE 128

enum game_status
{
  NEW_GAME,
  WAITING,
  RUNNING,
  END_GAME
};

enum player_status
{
  IN_LOBBY,
  REQUESTING,
  IN_GAME
};

typedef struct ServerStruct 
{
  int sd;
  socklen_t addrlen;
  struct sockaddr_in address;
  unsigned short int opt;

}Server;

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
  pthread_t player_tid;
  int player_sd;
  struct PlayerNode *next_node;
};

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
};

extern pthread_mutex_t player_mutex;
extern pthread_mutex_t game_mutex;

extern struct PlayerNode *player_head;
extern struct GameNode *game_head;

extern char ERROR;
extern char NO_ERROR;

extern const char NONE;
extern const char WON;
extern const char LOST;
extern const char DRAW;

#endif
