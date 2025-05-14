#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <signal.h>
#include "./datastructure.h"
#include "./messages.h"

#define PORT 8080

Server init_server();

void* player_thread( void *sd );

struct PlayerNode* register_player( const int player_sd );
char* check_player( const int player_sd );
bool player_found( const char *player_name );


struct PlayerNode *player_head;
struct GameNode *game_head;

pthread_mutex_t player_mutex;
pthread_mutex_t game_mutex;

#endif
