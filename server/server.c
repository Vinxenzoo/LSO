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

}


struct PlayerNode* register_player( const int player_sd )
{
   char *player_name = check_player( player_sd );
   struct PlayerNode *player = add_player( player_name );
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
         free( player_sd );
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
   player->player_id = pthread_self();
   player->player_sd = player_sd;
   player->champion = false;

   pthread_mutex_lock( &player_mutex );
   player->next_node = player_head;

   player_head = player;
   pthread_mutex_unlock( &player_mutex );

   return player;
}

//////////////////////////////////
//////////////////////////////////


void lobby(struct PlayerNode *player_data)
{

    const int player = dati_giocatore -> player;
    bool matched = true;   

    char buff-in[MAXIN]; //contiene le "scelte" del giocatore
    char buff-out[MAXOUT]; //contiene tutte le statistiche del giocatore formattate in un'unica stringa

    do
    {
      /*
        memset(buff-in, 0, MAXIN);
        memset(buff-out, 0, MAXOUT);
        player_data -> status = IN_LOBBY;
        //conversione in stringhe delle statistiche del giocatore
        char wins[3]; sprintf(wins, "%u", player_data -> wins);
        char losts[3]; sprintf(losts, "%u", player_data -> losts);
        char draws[3]; sprintf(draws, "%u", player_data -> draws);

        buff-out[0] = '\n'; //invio della stringa statistiche
        strcat(buff-out, player_data -> name);
        strcat(buff-out, "\nWINS: "); strcat(buff-out, wins);
        strcat(buff-out, "\nsconfitte: "); strcat(buff-out, losts);
        strcat(buff-out, "\npareggi: "); strcat(buff-out, draws);
        if (send(playr, buff-out, strlen(buff-out), MSG_NOSIGNAL) < 0) error_handler(player);
       
        */

        send_game();
        do //recv può essere interrotta da un segnale e restituire EINTR come errore
        {  //questo ciclo gestisce l'errore ed evita il crash del client
            if (errno == EINTR) errno = 0;
            if (recv(player, buff-in, MAXIN, 0) <= 0 && errno != EINTR) error_handler(player);
        } while (errno == EINTR);
        
        int index = 0;
        while (buff-in[index] != '\0') //case insensitive
        {
            buff-in[index] = toupper(buff-in[index]);
            index++;
        }
        if (strcmp(buff-in, "QUIT") == 0) matched = false;
        else if (strcmp(buff-in, "CREATE") == 0) 
        {
            do
            {   //crea e gioca la partita come proprietario
                struct GameNode *GameNode = // crea_partita_in_testa(player_data -> name, player);
                if (GameNode != NULL) 
                {
                    play_game(GameNode);
                    delete_game(GameNode);
                }
                else
                {
                    if (send(player, "impossible to create match\n", 49, MSG_NOSIGNAL) < 0) error_handler(player);
                }
            } while (player_data -> champion && !quit(player));
            //partita finita
            if (send(player, "Return in lobby\n", 17, MSG_NOSIGNAL) < 0) error_handler(player);
        }
        else 
        {   //invia la richiesta di unione alla partita, se accettata si blocca sulla propria cond_var fino a fine partita
            int i = atoi(buff-in);
            struct GameNode *match = NULL;
            if (i != 0) match = (trova_partita_da_indice(i));

            if (match == NULL)
            {
                if (send(player, "there isn't a match\n", 20, MSG_NOSIGNAL) < 0) error_handler(sd_giocatore);
            }
            else
            {   //gestisce la richiesta alla partita
                player_data -> status = REQUESTING;
                if (!accept_match(match, player, player_data -> name))
                {
                    if (match -> union == false)
                        {if (send(player, "Request refused\n", 30, MSG_NOSIGNAL) < 0) error_handler(player);}
                    else   
                        {if (send(player, "Another player accepted match\n", 64, MSG_NOSIGNAL) < 0) error_handler(player);}
                }
                else //richiesta accettata
                {
                    player_data -> status = IN_GAME;
                    pthread_mutex_lock(&(player_data -> status_mutex));

                    while (player_data -> status == IN_GAME)
                    {
                        //attende un segnale di fine partita
                        pthread_cond_wait(&(player_data -> stato_cv), &(play_game -> status_mutex));
                    }
                    pthread_mutex_unlock(&(player_data -> status_mutex));

                    while (player_data -> champion && !quit(player)) //se vince diventa il proprietario
                    {
                        struct GameNode *gameNode = crea_partita_in_testa(player_data -> name, player);
                        if (gameNode != NULL) 
                        {
                            play_game(gameNode);
                            delete_game(gameNode);
                        }
                        else
                        {
                            if (send(player, "Impossible to create match, attention please...\n", 49, MSG_NOSIGNAL) < 0) error_handler(player);
                        }
                    }
                    if (send(player, "Return in Lobby\n", 17, MSG_NOSIGNAL) < 0) error_handler(player);
                }
            }
        }
    } while (matched);
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



