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
