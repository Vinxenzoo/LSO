#include "server.h"

Server server_init()
{
   Server newServer;
   newServer.opt = 1;
   newServer.addrlen = sizeof(struct sockaddr_in); 

   if( ( newServer.fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )  //Domain=Address Family(ip+port), Type=BydirectionalConnection Protocol=Default Protocol
   {
      perror( "Socket creation failed" );
      exit( EXIT_FAILURE );
   }

   if( setsockopt( newServer.fd , SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &newServer.opt, sizeof( newServer.opt ) ) ) //SocketDescriptor, ProtocolLevel=SocketLevel, OptionName=ReuseLocalAddresses||ReuseLocalPort, OptionValue=true(1), Size=SizeOfOpt
   {
      perror( "Socket setting options failed.");
      exit( EXIT_FAILURE );
   }

   newServer.address.sin_family = AF_INET;
   newServer.address.sin_addr.s_addr = INADDR_ANY; //Ip address of the socket: with INADDR_ANY, it will accept all incoming addresses
   newServer.address.sin_port = htons ( PORT ); //htons convert the number 8080 from host to network byte order

   if( bind( newServer.fd, ( struct sockaddr* ) &newServer.address, sizeof( newServer.address ) ) < 0 )
   {
      perror( "Socket bind failed.");
      exit( EXIT_FAILURE );
   }

   if( listen( newServer.fd, 3 ) < 0) 
   {
      perror( "Socket listen failed." );
      exit( EXIT_FAILURE );
   }

   return newServer;

}

void* player_thread( void *sd )
{
   const int player_sd = *( ( int * ) sd );
   struct PlayerNode *player = register_player( player_sd );

   player_lobby();

}


struct PlayerNode* register_player( const int player_sd )
{
   char *player_name = check_player( player_sd );
   struct PlayerNode *player = add_player( player_name, player_sd );
   free( player_name );
   return player; 
}

char* check_player( const int player_sd )
{
   char *player_name = ( char * ) malloc( MAX_NAME_PLAYER * sizeof( char ) );

   if( player_name == NULL)
   {
      send( player_sd, INVALID_PLAYER_NAME, 3, MSG_NOSIGNAL );
      close( player_sd );
      pthread_exit( NULL );
   }

   if( send( player_sd, INSERT_PLAYER_NAME, 3, MSG_NOSIGNAL ) < 0 )
   {
      close( player_sd );
      free( player_name );
      pthread_exit( NULL );
   }

   bool name_found = false;

   do
   {
      memset( player_name, 0, MAX_NAME_PLAYER);

      if( recv( player_sd, player_name, MAX_NAME_PLAYER, 0 ) <= 0 )
      {
         close( player_sd );
         free( player_name );
         printf( "Check Error.\n" );
         pthread_exit( NULL );
      }

      if( !player_found( player_name ) )
      {
         name_found = true;
      }
      else if( send( player_sd, ALREADY_TAKEN_NAME, 3, MSG_NOSIGNAL) < 0 )
      {
         close( player_sd );
         free( player_name );
         printf( "Name already taken.\n" );
         pthread_exit( NULL );
      }
   }while( !name_found );

   if( send( player_sd, REGISTRATION_COMPLETED, 3, MSG_NOSIGNAL ) < 0 )
   {
      close( player_sd );
      free( player_name );
      printf( "Check failed.\n" );
      pthread_exit( NULL );
   }

   return player_name;
}

bool player_found( const char *player_name )
{
   pthread_mutex_lock( &player_mutex );
   struct PlayerNode *player = player_head;

   while( player != NULL )
   {
      if( strcmp( player->name, player_name ) == 0 )
      {
         pthread_mutex_unlock( &player_mutex );
         return true;
      }

      player = player->next_node;

   }

   pthread_mutex_unlock( &player_mutex );
   return false;

}

struct PlayerNode* add_player( const char *player_name, const int player_sd )
{
   struct PlayerNode *player = ( struct PlayerNode * ) malloc( sizeof ( struct PlayerNode ) );

   if( player == NULL )
   {
      send( player_sd, SERVER_ERROR, 3, MSG_NOSIGNAL );
      close( player_sd );
      printf( "Memory Allocation Failed.\n" );
      pthread_exit( NULL );
   }

   memset( player, 0, sizeof( struct PlayerNode ) );
   strcpy( player->name, player_name );
   player->wins = 0;
   player->losts = 0;
   player->draws = 0;
   pthread_mutex_init( &( player->mutex_state ), NULL );
   pthread_cond_init( &( player->cv_state ), NULL );
   player->status = IN_LOBBY;
   player->player_tid = pthread_self();
   player->player_sd = player_sd;
   player->champion = false;

   pthread_mutex_lock( &player_mutex );
   player->next_node = player_head;

   player_head = player;
   pthread_mutex_unlock( &player_mutex );

   return player;
}

void player_lobby()
{
   pthread_mutex_lock( &player_mutex );
   struct PlayerNode *player = player_head;
   pthread_mutex_unlock( &player_mutex );

   const pthread_t sender_tid = pthread_self();
   pthread_t receiver_tid = 0;

   while( player != NULL )
   {
      receiver_tid = player->player_tid;

      if(receiver_tid != sender_tid && ( player->status == IN_LOBBY ) )
      {
         pthread_kill( receiver_tid, SIGUSR2 );
      }

      player = player->next_node; 

   }
}


void lobby_handler( struct PlayerNode *player )
{
   char in_buffer[ MAX_MESSAGE_SIZE ];
   char out_buffer[ MAX_MESSAGE_SIZE ];

   const int player_sd = player->player_sd;
   bool connected = true;

   do
   {
      memset( in_buffer, 0, MAX_MESSAGE_SIZE );
      memset( out_buffer, 0, MAX_MESSAGE_SIZE );
      player->status = IN_LOBBY;

      char wins[3];
      sprintf( wins, "%u", player->wins );
      char losts[3];
      sprintf( losts, "%u", player->losts );
      char draws[3];
      sprintf( draws, "%u", player->draws );

      out_buffer[0] = '\n';
      strcat( out_buffer, "s" );
      strcat( out_buffer, wins ); strcat( out_buffer, "-" );
      strcat( out_buffer, losts ); strcat( out_buffer, "-" );
      strcat( out_buffer, draws ); strcat( out_buffer, "\n");

      if( send( player_sd, out_buffer, strlen( out_buffer ), MSG_NOSIGNAL ) < 0 )
      {
         printf( "Error sending stats.\n" );
      }

      do
      {
         if( errno == EINTR ) //Catch unexpected failures
         {
            errno = 0; //Avoid not expected failures
         }
         if( recv( player_sd, in_buffer, MAX_MESSAGE_SIZE, 0 ) <= 0 && errno != EINTR )
	 {
            printf("Generic Error in Lobby.\n");
         }
      }while( errno == EINTR );

      if( strcmp( in_buffer, EXIT ) == 0 )
      {
         connected = false
      }
      else if( strcmp( in_buffer, CREATE ) == 0 )
      {
         do
         {
            struct GameNode *game = new_game( player->name, player_sd );

            if( game != NULL )
            {
               play_game( game );
               delete_game( game );
            }
            else
            {
               if( send( player_sd, CREATE_GAME_FAILED, 3, MSG_NOSIGNAL ) < 0 )
               {
	          printf( "Game creation failed.\n" );
               }
            }
         } while( player->champion && !quit( player_sd );

         if( send( player_sd, RETURN_LOBBY, 3, MSG_NOSIGNAL ) < 0 )
         {
            printf( "Return to lobby failed.\n" );
         }
      }
      else
      {
         int index = atoi( in_buffer);
         struct GameNode *fgame = NULL;

         if( index != 0 )
         {
            fgame = ( find_game_by_index( index ) );
         }

         if( fgame == NULL )
         {
            if( send( player_sd, GAME_NOT_FOUND, 3, MSG_NOSIGNAL ) < 0 )
	    {
               printf( "Game not found.\n");
            }
         }
         else
         {
            player->status = REQUESTING;

            if( !game_accepted( fgame, player_sd, player->name ) )
            {
               if( fgame->join_reques )
	       {
		  if( send( player_sd, ANOTHER_PLAYER_REQUEST, 3, MSG_NOSIGNAL ) < 0 ) 
		  {
		     printf( "Another player request.\n" );
		  }
               }
               else
               {
                  if( send( player_sd, JOIN_REQUEST_REJECTED, 3, MSG_NOSIGNAL ) < 0 )
		  {
		     printf( "Join request rejected.\n" );
		  }
               }
            }
            else
            {
               player->status = IN_GAME;
	       pthread_mutex_lock( &( player->mutex_state ) );

	       while( player->status == IN_GAME )
	       {
		  pthread_cond_wait( &( player->cv_status), &( player->mutex_state ) )

	       }

	       pthread_mutex_unlock( &( player->mutex_state ) );

	       while( player->champion && !quit( player_sd ) )
	       {
		  struct GameNode *game = new_game( player->name, player_sd );

		  if( game != NULL)
		  {
		     play_game( game );
		     delete_game( game );
		  }
		  else
		  {
		     if( send( player_sd, CREATE_GAME_FAILED, 3, MSG_NOSIGNAL ) < 0 )
		     {
		        printf(" Rematch failed.\n");
                     }
		  }
	       }
	       if( send( player_sd, RETURN_LOBBY, 3, MSG_NOSIGNAL ) < 0 )
	       {
	          printf( "Returning to lobby because match failed.\n" );
               }
            }
         }
      }
   }while( connected );
}

bool accept_game( struct GameNode *game, const int enemy_sd, const char *enemy_name )
{
   if( game->join_request )
   {
      return false;
   }

   const int owner_sd = game->owner_sd;
   char buffer[ MAX_MESSAGE_SIZE ];
   memset( buffer, 0, MAX_MESSAGE_SIZE );
   strcat( buffer, 
}

struct GameNode* new_game( const char *owner_name, const int owner_sd )
{
   struct GameNode *game = ( struct GameNode * ) malloc( sizeof( struct GameNode ) );

   if( game == NULL )
   {
      return NULL;
   }

   memset( game, 0, sizeof( struct GameNode ) );
   strcpy( game->owner, owner_name );
   game->owner_sd = owner_sd;
   pthread_mutex_init( &( game->mutex_state ), NULL );
   pthread_cond_init( &( game->cv_state ), NULL );
   game->status = NEW_GAME;
   game->join_request = false;
   pthread_mutex_lock( &game_mutex );

   game->next_node = game_head;

   if( game_head != NULL && game_head->status == NEW_GAME )
   {
      game_head->status = WAITING;
   }

   game_head = game;
   pthread_mutex_unlock( &game_mutex );

   return game;
}

struct GameNode* find_game_by_player( const int player_sd )
{
   struct GameNode * game = game_head;

   while( game != NULL )
   {
      if( game->owner_sd == player_sd || game->enemy_sd == player_sd )
      {
          return game;
      }

      game = game->next_node;

   }

   return NULL;

}

struct GameNode* find_game_by_index( const unsigned int index )
{
   unsigned int game_index = 0;
   pthread_mutex_lock( &game_mutex );

   struct GameNode *game = game_head;

   while( game != NULL )
   {
      if( game->status == WAITING || game->status == NEW_GAME )
      {
         game_index++;
      }

      if( game_index == index )
      {
         pthread_mutex_unlock( &game_mutex );
         return game;
      }

      game = game->next_node;

   }

   pthread_mutex_unlock( &game_mutex );
   return NULL;

}

void delete_game( struct GameNode *game )
{
   pthread_mutex_lock( &game_mutex );

   if( game != NULL && game_head == game )
   {
      game_head = game_head->next_node;
      free( game );
      //segnala_cambiamento_partite()
   }
   else if ( game != NULL )
   {
      struct GameNode *tmp = game_head->next_node;

      while( tmp != NULL && tmp->next_node != game )
      {
         tmp = tmp->next_node;
      }

      if( tmp != NULL )
      {
         tmp->next_node = game->next_node;
         free( game );
         //segnala_cambiamento_partite();
      }
   }

   pthread_mutex_unlock( &game_mutex );

}

bool match_accepted(struct GameNode *match, const int opponent, const char *name_opp)
{
    if (match -> union == true) return false;
    const int host = match -> host;
    char buf[MAXOUT];
    memset(buf, 0, MAXOUT);
    strcat(buf, name_opp); strcat(buf, " want match at your game, Do you accept? [s/n]\n");

    char response = '\0'; //si occupa il codice client di verificare l'input

    if (send(opponent, "attend host...\n", 30, MSG_NOSIGNAL) < 0) error_handler(opponent);
    partita -> richiesta_unione = true;

    if (send(opponent, buf, strlen(buf), MSG_NOSIGNAL) < 0) 
    {
        error_handler(host);
        return false;
    }
    if (recv(host, &esponse, 1, 0) <= 0)
    {
        error_handler(opponent);
        return false;
    }

    resp = toupper(res); //case insensitive
    if (res == 'S')
    {
        strcpy(match -> opponent, name_opp);
        match -> opponent = opponent;
        if (send(host, "*** starting match by host***\n", 44, MSG_NOSIGNAL) < 0) error_handler(host);
        if (send(opponent, "*** starting match by opponent***\n", 42, MSG_NOSIGNAL) < 0) error_handler(sd_avversario);
        if (match != NULL)
        {
            match -> status = IN_GAME;
            match -> union = false;
            pthread_cond_signal(&(match -> stato_cv));
        }
        return true;
    }
    else
    {
        if (send(host, "Request refused, searching another opponent...\n", 51, MSG_NOSIGNAL) < 0) error_handler(player);
        match -> union = false;
    }
    return false;
}


void play_game(struct GameNode *gameData)
{
    const int host = gameData -> host;
    struct nodo_giocatore *host = trova_giocatore_da_sd(host);
    players -> status = IN_GAME;
    players -> champion = true;
    if (send(host, "waiting opponent...\n", 30, MSG_NOSIGNAL) < 0) error_handler(host);
    segnala_cambiamento_partite();

    pthread_mutex_lock(&(gameData -> mutex_state));
    while (gameData -> status != Running)
    {
        //controllo periodico in caso il proprietario si disconnetta mentre la partita è in attesa
        struct timespec waiting_time;
        clock_gettime(CLOCK_REALTIME, &waiting_time);
        tempo_attesa.tv_sec += 1;
        tempo_attesa.tv_nsec = 0;
        int all_results = pthread_cond_timedwait(&(gameData -> cv_state), &(gameData -> mutex_state), &waiting_time);
        if (all_results  == ETIMEDOUT) 
        { 
            //messaggio nullo che controlla se la socket è ancora attiva
            if (send(,host "\0", 1, MSG_NOSIGNAL) < 0) error_handler(host);
        }
    }
    pthread_mutex_unlock(&(gameData -> mutex_state));

    const int opponent = gameData -> opponent;
    struct PlayerNode *opponentt = trova_giocatore_da_sd(opponent);
    opponentt -> champion = false;

    int round = 0;

    do //si esce dal ciclo quando viene rifiutata la rivincita o se non c'è un pareggio
    {
        gameData -> status = RUNNING;
        round++;
        sign_change_game();

        bool err = false; //rappresenta un errore di disconnesione
        char play = '\0';
        char host_result = NONE; //il codice del client cambia il valore di questa variabile quando la partita finisce
        char opponent_result = NONE;
        //'0' = ancora in corso '1' = vince proprietario, '2' = vince avversario, '3' = pareggio

        //inizia la partita
        do
        {   //di default il server invia NOERROR per segnalare che è tutto ok, altrimenti è l'error handler a mandare ERROR all'altro giocatore
            //inoltre; visto che la partita è gestita dal thread proprietario, l'error handler si occupa anche di sbloccare l'avversario
            //in caso fosse il proprietario a disconnettersi
            if (round%2 != 0)
            {
                //inizia il proprietario
                if (recv(host, &play, 1, 0) <= 0) {error_handler(host); }
                if (recv(host, &host_result, 1, 0) <= 0) {error_handler(host); }
                if (send(opponent, &NOERROR, 1, MSG_NOSIGNAL) < 0) {error_handler(opponent); err = true; break;}
                if (send(opponent, &play, 1, MSG_NOSIGNAL) < 0) {error_handler(opponent); err = true; break;}
                if (host_result != NONE) break;

                //turno dell'avversario
                if (recv(opponent, &play, 1, 0) <= 0) {error_handler(opponent); err = true; break;}
                if (recv(opponent, &opponent_result, 1, 0) <= 0) {error_handler(opponent); err = true; break;}
                if (send(host, &NOERROR, 1, MSG_NOSIGNAL) < 0) {error_handler(host); }
                if (send(host, &play, 1, MSG_NOSIGNAL) < 0) {error_handler(host); }
            }
            else 
            {
                //inizia l'avversario
                if (recv(opponent, &play, 1, 0) <= 0) {error_handler(opponent); err = true; break;}
                if (recv(opponent, &opponent_result, 1, 0) <= 0) {error_handler(opponent); err = true; break;}
                if (send(host, &NOERROR, 1, MSG_NOSIGNAL) < 0) {error_handler(host); }
                if (send(host, &play, 1, MSG_NOSIGNAL) < 0) {error_handler(host); }
                if (opponent_result != NESSUNO) break;

                //turno del proprietario
                if (recv(host, &play, 1, 0) <= 0) {error_handler(host); }
                if (recv(host, &host_result, 1, 0) <= 0) {error_handler(host); }
                if (send(opponent, &NOERROR, 1, MSG_NOSIGNAL) < 0) {error_handler(opponent); err = true; break;}
                if (send(opponent, &play, 1, MSG_NOSIGNAL) < 0) {error_handler(opponent); err = true; break;}
            }
        } while (host_result == NONE && opponent_result == NONE);

        //in caso di errore si aggiornano le vittorie e si esce dalla partita
        if (err) 
        {
            host -> wins++;
            host -> champion = true;
            break;
        }

        //si aggiornano i contatori dei giocatori
        if (host_result == WIN || opponent_result == LOSE)
        {
            host -> wins++;
            opponent -> losts++;
            host -> champion = true;
            opponent -> champion = false;
            opponent -> status = IN_LOBBY;
            pthread_cond_signal(&(opponent -> cv_state));
            break;
        }
        else if (host_result == LOSE || opponent_result == WIN)
        {
            host -> losts++;
            opponent -> wins++;
            host -> champion = false;
            opponent -> champion = true;
            opponent -> status = REQUESTING;
            pthread_cond_signal(&(opponent -> cv_state));
            break;
        }

        host -> draws++;
        opponent -> draws++;

        gameData -> status = END_GAME;
        sign_change_game();

    //partita finita, rimane in stato terminata finchè la rivincita viene accettata o rifiutata
    } while (rematch(host, opponent));
}

bool rematch(const int host, const int opponent)
{
    struct PlayerNode *opponent = find_player(opponent);
    char opponent_response = '\0';
    char host_response = '\0';

    if (send(opponent, "Rematch? [s/n]\n", 17, MSG_NOSIGNAL) < 0) error_handler(opponent);
    if (recv(opponent, &opponent_response, 1, 0) <= 0) error_handler(opponent);
    
    if (opponent_response != 'S') {if (send(host, "Rematch refused\n", 36, MSG_NOSIGNAL) < 0) error_handler(host);}
    else //avversario vuole rivincita
    {
        if (send(opponent, "Waiting host...\n", 30, MSG_NOSIGNAL) < 0) error_handler(opponent);
        if (send(host, "do you want a rematch? [s/n]\n", 48, MSG_NOSIGNAL) < 0) error_handler(host);
        if (recv(host, &host_response, 1, 0) <= 0) error_handler(host);
    }
    if (host_response != 'S') //si torna alla lobby
    {
        if (host_response == 'N')
        {
            if (send(opponent, "Rematch refused\n", 37, MSG_NOSIGNAL) < 0) error_handler(opponent);
        }
        if (opponent != NULL)
        {
            opponent -> status = REQUESTING;
            pthread_cond_signal(&(opponent -> cv_state));
        }
        return false;
    }
    else
    {
        if (send(opponent, "Rematch accepted, ready for the next rpund\n", 50, MSG_NOSIGNAL) < 0) error_handler(opponent);
        if (send(host, "Rematch accepted, ready for the next rpund\n", 50, MSG_NOSIGNAL) < 0) error_handler(host);
        return true;
    }
}

bool quit(const int host)
{
    char response = '\0';
    if (send(host, "Do ypu want to accept another player? [s/n]\n", 40, MSG_NOSIGNAL) < 0) error_handler(host);
    if (recv(host, &response, 1, 0) <= 0) error_handler(host);
    response = toupper(response);
    if (response == 'S') return false;
    else return true;
}


struct PlayerNode* playerCreate_Head(const char *player_name, const int client)
{
    struct PlayerNode *newHead = (struct PlayerNode *) malloc(sizeof(struct PlayerNode));
    if (newHead == NULL)
    {
        send(client, "error\n", 7, MSG_NOSIGNAL); 
        close(client);
        printf("error for creating player\n");
        pthread_exit(NULL);
    }

    memset(newHead, 0, sizeof(struct PlayerNode)); //pulisce il nodo per sicurezza
    strcpy(newHead -> name, PlayerNode);
    newHead -> wins = 0;
    newHead -> losts = 0;
    newHead -> draws = 0;
    pthread_mutex_init(&(newHead -> mutex_state), NULL);
    pthread_cond_init(&(newHead -> cv_state), NULL);
    newHead -> status = IN_LOBBY;
    newHead -> player_tid = pthread_self();
    newHead -> player_sd = client;
    newHead -> champion = false;

    pthread_mutex_lock(&player_mutex);
    newHead -> next_node = player_head;

    player_head = newHead;
    pthread_mutex_unlock(&player_mutex);

    return newHead;
}

struct PlayerNode* findPlayer_socket_desc(const int soc_desc)
{
    struct PlayerNode *temporaneo = player_head;

    while (temporaneo != NULL)
    {
        if (temporaneo -> player_sd == soc_desc) return temporaneo;
        temporaneo = temporaneo -> next_node;
    } 
    return NULL; //improbabile
}

struct PlayerNode* findPlayer_tid(const pthread_t tid)
{
    struct PlayerNode *temporaneo = testa_giocatori;

    while (temporaneo != NULL)
    {
        if (temporaneo -> tid_giocatore == tid) 
        {
         return temporaneo;
        }
        temporaneo = temporaneo -> next_node;
    }
    return NULL; //improbabile
}

void delete_player(struct PlayerNode *node)
{
    pthread_mutex_lock(&player_mutex);
    if (node != NULL && player_head == node) //significa che si sta cercando di cancellare la testa
    {
        player_head = player_head -> next_node;
        free(node);
    }
    else
    { 
      if (node != NULL)
         {
            struct PlayerNode *temporaneo = player_head;
            while(temporaneo -> next_node != node && temporaneo != NULL) //in teoria è impossibile che tmp diventi null
            {
                  temporaneo = temporaneo -> next_node;
            }
            temporaneo -> next_node = node -> next_node;
            free(node);
         }
         pthread_mutex_unlock(&player_mutex);
      }
}

