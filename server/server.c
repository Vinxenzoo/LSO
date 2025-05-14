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
