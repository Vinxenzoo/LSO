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

struct PlayerNode* register_player( const int );
struct PlayerNode* add_player( const char *, const int );
char* check_player( const int );
bool player_found( const char * );
void player_lobby();
void lobby_handler( struct PlayerNode * );
bool accept_game( struct GameNode *, const int , const char * ); 

struct GameNode* new_game( const char*, const int );
struct GameNode* find_game_by_sd( const int );
struct GameNode* find_game_by_index( const unsigned int );
void delete_game( struct GameNode * );

struct PlayerNode *player_head;
struct GameNode *game_head;

pthread_mutex_t player_mutex;
pthread_mutex_t game_mutex;

void lobby(struct PlayerNode *player_data);
//se il proprietario accetta la richiesta di unione alla partita inserisce i dati dell'avversario nel nodo partita e restituisce vero, falso altrimenti
bool match_accepted(struct GameNode *match, const int opponent, const char *opponent_name);

void play_game(struct GameNode *gameData);
bool rematch(const int host, const int opponent);
bool quit(const int player_sd);

struct PlayerNode* playerCreate_Head(const char *player_name, const int client);
struct PlayerNode* findPlayer_socket_desc(const int soc_desc)
struct PlayerNode* findPlayer_tid(const pthread_t tid);
void delete_player(struct PlayerNode *node);


void err_handler(const int player);
void sigalrm_h();
void docker_SIGTERM_h();
void show_game_changement();
void send_game();
void show_new_player()
void handler_newPlayer();

#endif
