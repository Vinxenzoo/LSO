#include "server.h"

int inizializza_server() //crea la socket, si mette in ascolto e restituisce il socket descriptor
{
    int sd;
    int opt = 1; //1 = abilita, 0 = disabilita
    struct sockaddr_in address;
    socklen_t lenght = sizeof(struct sockaddr_in);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket creation error"), exit(EXIT_FAILURE);

    //imposta la socket attivando SO_REUSEADDR che permette di riavviare velocemente il server in caso di crash o riavvii
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        perror("Errore setsockopt");
        close(sd);
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, lenght);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sd, (struct sockaddr *) &address, lenght) < 0)
        perror("binding error"), exit(EXIT_FAILURE);

    if (listen(sd, 5) < 0)
        perror("listen error"), exit(EXIT_FAILURE);

    return sd;
}

/*
Server server_init()
{
   Server newServer;
   newServer.opt = 1;
   newServer.addrlen = sizeof(struct sockaddr_in); 

   if( ( newServer.sd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )  //Domain=Address Family(ip+port), Type=BydirectionalConnection Protocol=Default Protocol
   {
      perror( "Socket creation failed" );
      exit( EXIT_FAILURE );
   }

   if( setsockopt( newServer.sd , SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &newServer.opt, sizeof( newServer.opt ) ) ) //SocketDescriptor, ProtocolLevel=SocketLevel, OptionName=ReuseLocalAddresses||ReuseLocalPort, OptionValue=true(1), Size=SizeOfOpt
   {
      perror( "Socket setting options failed.");
      exit( EXIT_FAILURE );
   }

   newServer.address.sin_family = AF_INET;
   newServer.address.sin_addr.s_addr = INADDR_ANY; //Ip address of the socket: with INADDR_ANY, it will accept all incoming addresses
   newServer.address.sin_port = htons ( PORT ); //htons convert the number 8080 from host to network byte order

   if( bind( newServer.sd, ( struct sockaddr* ) &newServer.address, sizeof( newServer.address ) ) < 0 )
   {
      perror( "Socket bind failed.");
      exit( EXIT_FAILURE );
   }

   if( listen( newServer.sd, 3 ) < 0) 
   {
      perror( "Socket listen failed." );
      exit( EXIT_FAILURE );
   }

   return newServer;

}
*/

void* player_thread( void *sd )
{
   printf("sono in player thread\n");
   const int player_sd = *( ( int * ) sd );
   struct PlayerNode *player = register_player( player_sd );
   show_new_player();
   lobby_handler(player);
   printf("sono sotto player lobby\n");

   close( player_sd );
   delete_player( player );
   pthread_exit( NULL );
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
   
    char inbuffer[MAX_MESSAGE_SIZE]; //contiene le "scelte" del giocatore
    char outbuffer[MAX_MESSAGE_SIZE]; //contiene tutte le statistiche del giocatore formattate in un'unica stringa

    const int sd_giocatore = player -> player_sd;
    bool connesso = true;   
    do
    {
        memset(inbuffer, 0, MAX_MESSAGE_SIZE);
        memset(outbuffer, 0, MAX_MESSAGE_SIZE);
        player -> status = IN_LOBBY;
        //conversione in stringhe delle statistiche del giocatore
        char vittorie[3]; sprintf(vittorie, "%u", player -> wins);
        char sconfitte[3]; sprintf(sconfitte, "%u", player -> losts);
        char pareggi[3]; sprintf(pareggi, "%u", player -> draws);

        outbuffer[0] = '\n'; //invio della stringa statistiche
        strcat(outbuffer, player -> name);
        strcat(outbuffer, "\nvittorie: "); strcat(outbuffer, vittorie);
        strcat(outbuffer, "\nsconfitte: "); strcat(outbuffer, sconfitte);
        strcat(outbuffer, "\npareggi: "); strcat(outbuffer, pareggi);
        if (send(sd_giocatore, outbuffer, strlen(outbuffer), MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
       
        send_game();
        do //recv può essere interrotta da un segnale e restituire EINTR come errore
        {  //questo ciclo gestisce l'errore ed evita il crash del client
            if (errno == EINTR) errno = 0;
            if (recv(sd_giocatore, inbuffer, MAX_MESSAGE_SIZE, 0) <= 0 && errno != EINTR) err_handler(sd_giocatore);
        } while (errno == EINTR);
        
        int i = 0;
        while (inbuffer[i] != '\0') //case insensitive
        {
            inbuffer[i] = toupper(inbuffer[i]);
            i++;
        }
        printf("%s\n",inbuffer);
        if (strcmp(inbuffer, "ESCI") == 0) connesso = false;
        else if (strcmp(inbuffer, "CREA") == 0) 
        {
            do
            {   //crea e gioca la partita come proprietario
                struct GameNode *nodo_partita = new_game(player -> name, sd_giocatore);
                if (nodo_partita != NULL) 
                {
                    play_game(nodo_partita);
                    delete_game(nodo_partita);
                }
                else
                {
                    if (send(sd_giocatore, "Impossibile creare partita, attendi e riprova...\n", 49, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
                }
            } while (player -> champion && !quit(sd_giocatore));
            //partita finita
            if (send(sd_giocatore, "Ritorno in lobby\n", 17, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
        }
        else 
        {   //invia la richiesta di unione alla partita, se accettata si blocca sulla propria cond_var fino a fine partita
            int indice = atoi(inbuffer);
            struct GameNode *partita = NULL;
            if (indice != 0) partita = (find_game_by_index(indice));

            if (partita == NULL)
            {
                if (send(sd_giocatore, "Partita non trovata\n", 20, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
            }
            else
            {   //gestisce la richiesta alla partita
                player -> status = REQUESTING;
                if (!match_accepted(partita, sd_giocatore, player -> name))
                {
                    if (partita -> join_request == false)
                        {if (send(sd_giocatore, "Richiesta di unione rifiutata\n", 30, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);}
                    else   
                        {if (send(sd_giocatore, "Un altro giocatore ha già richiesto di unirsi a questa partita\n", 64, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);}
                }
                else //richiesta accettata
                {
                    pthread_mutex_lock(&(player -> mutex_state));
                    player -> status = IN_GAME;

                    while (player -> status == IN_GAME)
                    {
                        printf("CAPASSO1\n");
                        //attende un segnale di fine partita
                        pthread_cond_wait(&(player -> cv_state), &(player -> mutex_state));
                    }
                    pthread_mutex_unlock(&(player -> mutex_state));

                    while (player -> champion && !quit(sd_giocatore)) //se vince diventa il proprietario
                    {
                        struct GameNode *nodo_partita = new_game(player -> name, sd_giocatore);
                        if (nodo_partita != NULL) 
                        {
                            play_game(nodo_partita);
                            delete_game(nodo_partita);
                        }
                        else
                        {
                            if (send(sd_giocatore, "Impossibile creare partita, attendi e riprova...\n", 49, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
                        }
                    }
                    if (send(sd_giocatore, "Ritorno in lobby\n", 17, MSG_NOSIGNAL) < 0) err_handler(sd_giocatore);
                }
            }
        }
    } while (connesso);
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
   strcat( buffer, "r" );
   strcat( buffer, enemy_name );
   strcat( buffer, "\n");

   char response = '\0';

   if( send( enemy_sd, WAITING_OWNER_RESPONSE, 3, MSG_NOSIGNAL ) < 0 )
   {
      err_handler( owner_sd );
      return false;
   }
   if( recv( owner_sd, &response, 1, 0) <= 0 )
   {
      err_handler( owner_sd );
      return false;
   }

   if( response == 'S' )
   {
      strcpy( game->enemy, enemy_name );
      game->enemy_sd = enemy_sd;

      if( send( owner_sd, STARTING_GAME_OWNER, 3, MSG_NOSIGNAL) < 0 )
      {
         err_handler( owner_sd );
      }
      if( send( owner_sd, STARTING_GAME_ENEMY, 3, MSG_NOSIGNAL) < 0 )
      {
         err_handler( enemy_sd );
      }
      if( game != NULL )
      {
         game->status = RUNNING;
         game->join_request = false;
         pthread_cond_signal( &( game->cv_state ) );
      }
      
      return true;

   }
   else
   {
      if( send( owner_sd, REQUEST_DENIED, 3, MSG_NOSIGNAL ) )
      {
         err_handler( owner_sd );
      }

      game->join_request = false;
   
   }

   return false;

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
         printf("dido\n");
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
      show_game_changement();
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
         show_game_changement();
      }
   }

   pthread_mutex_unlock( &game_mutex );

}

bool match_accepted(struct GameNode *match, const int opponent, const char *name_opp)
{
   printf("avversario: %s\n", name_opp);
    if (match -> join_request == true) return false;
    const int host = match -> owner_sd;
    char buf[MAX_MESSAGE_SIZE];
    memset(buf, 0, MAX_MESSAGE_SIZE);
    strcat(buf, name_opp); strcat(buf, "Do you accept? [s/n]\n");

    char response = '\0'; //si occupa il codice client di verificare l'input

    if (send(opponent, "attend host...\n", 30, MSG_NOSIGNAL) < 0) err_handler(opponent);
    match -> join_request = true;

    if (send(host, buf, strlen(buf), MSG_NOSIGNAL) < 0) 
    {
        err_handler(host);
        return false;
    }
    if (recv(host, &response, 1, 0) <= 0)
    {
        err_handler(opponent);
        return false;
    }

    if (response == 'S')
    {
        strcpy(match -> enemy, name_opp);
        match -> enemy_sd = opponent;
        if (send(host, "~Host~\n", 44, MSG_NOSIGNAL) < 0) err_handler(host);
        if (send(opponent, "\n~Opponent~\n", 42, MSG_NOSIGNAL) < 0) err_handler(opponent);
        if (match != NULL)
        {
            match -> status  =  RUNNING;
            match -> join_request = false;
            pthread_cond_signal(&(match -> cv_state));
        }
        printf("CAPASSO\n");
        return true;
    }
    else
    {
        if (send(host, "Request refused, searching another opponent...\n", 51, MSG_NOSIGNAL) < 0) err_handler(host);
        match -> join_request = false;
    }
    return false;
}


void play_game(struct GameNode *gameData)
{
    const int host = gameData -> owner_sd;
    struct PlayerNode *owner = findPlayer_socket_desc(host);
    owner -> status = IN_GAME;
    owner -> champion = true;
    if (send(host, "waiting opponent...\n", 30, MSG_NOSIGNAL) < 0) err_handler(host);
    show_game_changement();

    pthread_mutex_lock(&(gameData -> mutex_state));
    while (gameData -> status != RUNNING )
    {
        //controllo periodico in caso il proprietario si disconnetta mentre la partita è in attesa
        struct timespec waiting_time;
        clock_gettime(CLOCK_REALTIME, &waiting_time);
        waiting_time.tv_sec += 1;
        waiting_time.tv_nsec = 0;
        int all_results = pthread_cond_timedwait(&(gameData -> cv_state), &(gameData -> mutex_state), &waiting_time);
        if (all_results  == ETIMEDOUT) 
        { 
            //messaggio nullo che controlla se la socket è ancora attiva
            if (send(host, "\0", 1, MSG_NOSIGNAL) < 0) err_handler(host);
        }
    }
    pthread_mutex_unlock(&(gameData -> mutex_state));

    const int opponent = gameData -> enemy_sd;
    struct PlayerNode *opponent_struct = findPlayer_socket_desc(opponent);
    opponent_struct -> champion = false;

   unsigned int round = 0;

    do //si esce dal ciclo quando viene rifiutata la rivincita o se non c'è un pareggio
    {
        gameData -> status = RUNNING;
        round++;
        show_game_changement();

        bool err = false; //rappresenta un errore di disconnesione
        char play = '\0';
        char host_result = NONE; //il codice del client cambia il valore di questa variabile quando la partita finisce
        char opponent_result = NONE; // oppure altro valore iniziale

        //'0' = ancora in corso '1' = vince proprietario, '2' = vince avversario, '3' = pareggio

        //inizia la partita
        do
        {   //di default il server invia NOERROR per segnalare che è tutto ok, altrimenti è l'error handler a mandare ERROR all'altro giocatore
            //inoltre; visto che la partita è gestita dal thread proprietario, l'error handler si occupa anche di sbloccare l'avversario
            //in caso fosse il proprietario a disconnettersi
            if (round%2 != 0)
            {
                //inizia il proprietario
                if (recv(host, &play, 1, 0) <= 0) {err_handler(host); } printf("Server owner %c", play);
                if (recv(host, &host_result, 1, 0) <= 0) {err_handler(host); }
                if (send(opponent, &NO_ERROR, 1, MSG_NOSIGNAL) < 0) {err_handler(opponent); err = true; break;}
                if (send(opponent, &play, 1, MSG_NOSIGNAL) < 0) {err_handler(opponent); err = true; break;}
                if (host_result != NONE) break;

                //turno dell'avversario
                if (recv(opponent, &play, 1, 0) <= 0) {err_handler(opponent); err = true; break;} printf("Server oppoent %c", play);
                if (recv(opponent, &opponent_result, 1, 0) <= 0) {err_handler(opponent); err = true; break;}
                if (send(host, &NO_ERROR, 1, MSG_NOSIGNAL) < 0) {err_handler(host); }
                if (send(host, &play, 1, MSG_NOSIGNAL) < 0) {err_handler(host); }
            }
            else 
            {
                //inizia l'avversario
                if (recv(opponent, &play, 1, 0) <= 0) {err_handler(opponent); err = true; break;}
                if (recv(opponent, &opponent_result, 1, 0) <= 0) {err_handler(opponent); err = true; break;}
                if (send(host, &NO_ERROR, 1, MSG_NOSIGNAL) < 0) {err_handler(host); }
                if (send(host, &play, 1, MSG_NOSIGNAL) < 0) {err_handler(host); }
                if (opponent_result != NONE) break;

                //turno del proprietario
                if (recv(host, &play, 1, 0) <= 0) {err_handler(host); }
                if (recv(host, &host_result, 1, 0) <= 0) {err_handler(host); }
                if (send(opponent, &NO_ERROR, 1, MSG_NOSIGNAL) < 0) {err_handler(opponent); err = true; break;}
                if (send(opponent, &play, 1, MSG_NOSIGNAL) < 0) {err_handler(opponent); err = true; break;}
            }
        } while (host_result == NONE && opponent_result == NONE);

        //in caso di errore si aggiornano le vittorie e si esce dalla partita
        if (err) 
        {
            owner -> wins++;
            owner -> champion = true;
            break;
        }

        //si aggiornano i contatori dei giocatori
        if (host_result == WON || opponent_result == LOST)
        {
            owner -> wins++;
            opponent_struct -> losts++;
            owner -> champion = true;
            opponent_struct -> champion = false;
            opponent_struct -> status = IN_LOBBY;
            pthread_cond_signal(&(opponent_struct -> cv_state));
            break;
        }
        else if (host_result == LOST || opponent_result == WON)
        {
            owner -> losts++;
            opponent_struct -> wins++;
            owner -> champion = false;
            opponent_struct -> champion = true;
            opponent_struct -> status = REQUESTING;
            pthread_cond_signal(&(opponent_struct -> cv_state));
            break;
        }

        owner -> draws++;
        opponent_struct -> draws++;

        gameData -> status = END_GAME;
        show_game_changement();
        printf("SONO DOPO SHOW GAME CHANGEMENT\n");
    //partita finita, rimane in stato terminata finchè la rivincita viene accettata o rifiutata
    } while (rematch(host, opponent));
}

bool rematch(const int host, const int opponent_sd)
{
    printf("REMATCH\n");
    struct PlayerNode *opponent = findPlayer_socket_desc(opponent_sd);
    char opponent_response = '\0';
    char host_response = '\0';

    if (send(opponent_sd, "Rematch? [s/n]\n", 17, MSG_NOSIGNAL) < 0) err_handler(opponent_sd);
    if (recv(opponent_sd, &opponent_response, 1, 0) <= 0) err_handler(opponent_sd);
    
    if (opponent_response != 'S') {if (send(host, "Rematch refused\n", 36, MSG_NOSIGNAL) < 0) err_handler(host);}
    else //avversario vuole rivincita
    {
        if (send(opponent_sd, "Waiting host...\n", 30, MSG_NOSIGNAL) < 0) err_handler(opponent_sd);
        if (send(host, "do you want a rematch? [s/n]\n", 48, MSG_NOSIGNAL) < 0) err_handler(host);
        if (recv(host, &host_response, 1, 0) <= 0) err_handler(host);
    }
    if (host_response != 'S') //si torna alla lobby
    {
        if (host_response == 'N')
        {
            if (send(opponent_sd, "Rematch refused\n", 37, MSG_NOSIGNAL) < 0) err_handler(opponent_sd);
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
        if (send(opponent_sd, "Rematch accepted, ready for the next rpund\n", 50, MSG_NOSIGNAL) < 0) err_handler(opponent_sd);
        if (send(host, "Rematch accepted, ready for the next rpund\n", 50, MSG_NOSIGNAL) < 0) err_handler(host);
        return true;
    }
}

bool quit(const int host)
{
    char response = '\0';
    if (send(host, "Do you want to accept another player? [s/n]\n", 60, MSG_NOSIGNAL) < 0) err_handler(host);
    if (recv(host, &response, 1, 0) <= 0) err_handler(host);
    
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
    strcpy(newHead -> name, player_name);
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
    struct PlayerNode *temporaneo = player_head;

    while (temporaneo != NULL)
    {
        if (temporaneo -> player_tid == tid) 
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


void show_game_changement()
{ 
    const pthread_t sender_tid = pthread_self();
    pthread_t recv_tid = 0;

    //per essere sicuri che la testa non cambi nel frattempo
    pthread_mutex_lock(&player_mutex);
    struct PlayerNode *momentary = player_head;
    pthread_mutex_unlock(&player_mutex);

    while (momentary != NULL)
    {
        recv_tid = momentary -> player_tid;
        if (recv_tid != sender_tid && momentary -> status == IN_LOBBY) pthread_kill(recv_tid, SIGUSR1);
        momentary = momentary -> next_node;
    }
}

void err_handler(const int player)
{
    pthread_t thread_id = 0;
    struct PlayerNode *player_tmp = findPlayer_socket_desc(player);
    if (player_tmp != NULL) thread_id = player_tmp -> player_tid;
    struct GameNode *match = find_game_by_player(player);

    if (match != NULL) 
    {
        if (match -> status == RUNNING)
        {   //le send non fanno error checking perchè in caso di errore si creerebbe una ricorsione infinita
            if (player == match -> owner_sd) 
            {
                send(match -> enemy_sd, &ERROR, 1, MSG_NOSIGNAL);
                struct PlayerNode *opponent = findPlayer_socket_desc(match -> enemy_sd);
                if (opponent != NULL)
                {
                    opponent -> wins++;
                    opponent -> champion = true;
                    opponent -> status = REQUESTING;
                    pthread_cond_signal(&(opponent -> cv_state));
                }
                delete_game(match); 
            }
            else send(match -> owner_sd, &ERROR, 1, MSG_NOSIGNAL);
        }
        else
        {
            delete_game(match); 
        }
    }
    if (player != NULL) pthread_kill(thread_id, SIGALRM);
}

void sigalrm_h()
{
    struct PlayerNode *player = findPlayer_tid(pthread_self());
    close(player -> player_sd);
    delete_player(player);
    pthread_exit(NULL);
}

void docker_SIGTERM_h()
{
    exit(EXIT_SUCCESS);
}

void send_game()
{
    struct PlayerNode *player = findPlayer_tid(pthread_self());
    const int client = player -> player_sd;

    pthread_mutex_lock(&game_mutex);
    struct GameNode *momentary = game_head;

    char buff_out[MAX_MESSAGE_SIZE];
    memset(buff_out, 0, MAX_MESSAGE_SIZE);

    int i = 0; //conta le partite a cui è possibile aggiungersi
    char index_str[3]; //l'indice verrà convertito in questa stringa
    memset(index_str, 0, 3);

    char game_state[28];

    if (momentary == NULL) {if (send(client, "\nthere aren't match actives now, write\"create\"for create ones \"esc\" to exit\n", 90, MSG_NOSIGNAL) < 0) err_handler(client);}
    else
    {
        if (send(client, "\n\nMATCH LIST\n", 15, MSG_NOSIGNAL) < 0) err_handler(client);
        while (momentary != NULL)
        {
            memset(buff_out, 0, MAX_MESSAGE_SIZE);
            memset(game_state, 0, 28);
            switch (momentary -> status)
            {
                case NEW_GAME:
                    strcpy(game_state, "Create now\n");
                    break;
                case WAITING:
                    strcpy(game_state, "waiting a player\n");
                    break;
                case RUNNING:
                    strcpy(game_state, "Running a game\n");
                    break;
                case END_GAME:
                    strcpy(game_state, "Ended a game\n");
                    break;
            }
            strcat(buff_out, momentary -> owner); strcat(buff_out, "\nmatch's "); 
            strcat(buff_out, momentary -> enemy); strcat(buff_out, "\nAvversario: ");
            strcat(buff_out, game_state); strcat(buff_out, "\nState's ");

            if (momentary -> status == NEW_GAME || momentary -> status== WAITING)
            {
                i++;
                  printf("\nID: %d", i);
                sprintf(index_str, "%u\n", i);
                strcat(buff_out, "ID: "); strcat(buff_out, index_str);
            }
            if (send(client, buff_out, strlen(buff_out), MSG_NOSIGNAL) < 0) err_handler(client);

            momentary = momentary -> next_node;
         }
        if (send(client, "\nUnisciti a una partita in attesa scrivendo il relativo ID, scrivi \"crea\" per crearne una o \"esci\" per uscire\n", 110, MSG_NOSIGNAL) < 0) err_handler(client);
    }
    pthread_mutex_unlock(&game_mutex);
}


void show_new_player()
{
    //per essere sicuri che la testa non cambi nel frattempo
    pthread_mutex_lock(&player_mutex);
    struct PlayerNode *momentary = player_head;
    pthread_mutex_unlock(&player_mutex);

    const pthread_t sender_tid = pthread_self();
    pthread_t recv_tid = 0;

    while (momentary != NULL)
    {
        recv_tid = momentary -> player_tid;
        if (recv_tid != sender_tid && (momentary -> status == IN_LOBBY)) pthread_kill(recv_tid, SIGUSR2);
        momentary = momentary -> next_node;
    }
}

void handler_newPlayer()
{
    pthread_mutex_lock(&player_mutex);
    struct PlayerNode *momentary = player_head;
    pthread_mutex_unlock(&player_mutex);

    char mess[MAX_MESSAGE_SIZE];
    memset(mess, 0, MAX_MESSAGE_SIZE);
    if (momentary != NULL) //controllo teoricamente inutile
    {
        strcat(mess, momentary -> name); 
        strcat(mess, "entered lobby!\n");

        struct PlayerNode *player = findPlayer_tid(pthread_self());

        if (send(player -> player_sd, mess, strlen(mess), MSG_NOSIGNAL) < 0) err_handler(player -> player_sd);
    }
}




