#ifndef function_h
#define function_h

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdbool.h>
#include "datastructures.h"

//si connette alla socket del server, restituisce il sd del client
void init_socket(); //CAMBIATA

//funzione che rappresenta il ciclo di vita del giocatore
void start_client_session(); //CAMBIATA
//start function del thread che scrive sulla socket
void* thread_writer(); //NON CAMBIATA

//funzione che gestisce la partita tra 2 giocatori, incluse eventuali rivincite
void play_games(char *inbuffer, const enum player_type type); //NON CAMBIATA

// Chiede all'utente di inserire una mossa valida (0–9), la valida, e la restituisce.
int get_valid_move(); //NUOVA
//aggiorna la griglia di gioco e il numero giocate, invia la giocata e l'esito della partita al server, restituisce l'esito
char send_move(unsigned short int *moves); //CAMBIATA
//riceve la giocata dal server, aggiorna la griglia, restituisce l'esito
char receive_move(unsigned short int *moves); //NON CAMBIATA

//controlla chi ha vinto e restituisce l'esito
char check_outcome(const unsigned short int *moves); //CAMBIATA
//controlla se il giocatore ha inserito un input valido
bool check_move(const int move); //NON CAMBIATA

//inserisce O nella griglia in base alla mossa del giocatore
void insertO(const unsigned short int move); //LEGGERMENTE CAMBIATA
//inserisce X nella griglia in base alla mossa dell'avversario
void insertX(const unsigned short int move); //LEGGERMENTE CAMBIATA
//stampa la griglia attuale
void print_grid(); //CAMBIATA

//gestiscono la richiesta di rivincita, restituendo true se la rivincita è stata accettata
bool rematch_host(); //NON CAMBIATA
bool rematch_opponent(); //NON CAMBIATA

//manda un messaggio di errore e chiude il processo
void error_handler(); 
//uccide il thread quando viene inviato il segnale SIGUSR1
void SIGUSR1_handler(); 
//gestisce SIGTERM inviato da docker quando stoppa i container
void SIGTERM_handler(); 

#endif