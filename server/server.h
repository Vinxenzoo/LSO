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

//funzione che gestisce il ciclo di vita di un giocatore
void lobby(struct PlayerNode *player_data);
//se il proprietario accetta la richiesta di unione alla partita inserisce i dati dell'avversario nel nodo partita e restituisce vero, falso altrimenti
bool match_accepted(struct GameNode *match, const int opponent, const char *opponent_name);

void play_game(struct GameNode *gameData);
bool rematch(const int host, const int opponent);
bool quit(const int player_sd);

#endif
