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

void init_socket();

void start_client_session();
void* thread_writer();

void play_games(char *inbuffer, const enum player_type type);

int get_valid_move(); 
char send_move(unsigned short int *moves); 
char receive_move(unsigned short int *moves);

char check_outcome(const unsigned short int *moves); 
bool check_move(const int move);


void insertO(const unsigned short int move); 
void insertX(const unsigned short int move); 

void print_grid(); 

bool rematch_host(); 
bool rematch_opponent();

void error_handler(); 
void SIGUSR1_handler();
void SIGTERM_handler(); 

#endif