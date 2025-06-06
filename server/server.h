#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <signal.h>
#include "./datastructures.h"

#define PORT 8080

int init_server();
struct PlayerNode* init_player( const int ); 

void* player_thread( void *sd );
void* handle_player_session( struct PlayerNode *, const int );
char* check_player_name( char *, const int );

bool player_found( const char * );

void player_lobby();
void lobby_handler( struct PlayerNode * );

void* manage_game( struct GameNode* , const int );

void show_game_changement();
void send_game();

struct GameNode* new_game( const char*, const int );
void* init_game( struct GameNode *, const char *, const int );
void delete_game( struct GameNode * );

struct GameNode* find_game_by_player( const int );
struct GameNode* find_game_by_index( const unsigned int );

bool match_accepted( struct GameNode *, const int , const char * );
void* setup_running_match( struct GameNode *, const int , const char *, const int );

void play_game( struct GameNode * );
bool rematch( const int , const int );
bool quit( const int );

struct PlayerNode* player_create_head( const char *, const int );
void* check_error_player_head(struct PlayerNode*, const int);
void* init_player_head(struct PlayerNode*, const int, const char*);

struct PlayerNode* find_player_socket_desc( const int );
struct PlayerNode* find_player_tid( const pthread_t );

void delete_player( struct PlayerNode * );
void* delete_node_beetween(struct PlayerNode*, struct PlayerNode*);

void show_new_player();
void handler_new_player();

void err_handler( const int );
void* handle_player_disconnection( struct GameNode*, const int);

void sigalrm_h();
void docker_SIGTERM_h();

#endif