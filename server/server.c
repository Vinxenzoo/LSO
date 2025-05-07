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
