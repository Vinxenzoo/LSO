#include "server.h"

pthread_mutex_t player_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

struct PlayerNode *player_head = NULL;
struct GameNode *game_head = NULL;

char ERROR = '1';
char NO_ERROR = '0';

const char NONE = '0';
const char WON = '1';
const char LOST = '2';
const char DRAW = '3';

int main()
{
    int sd = init_server();
    printf( "Server running on port 8080.\n" );

    int client_sd;
    struct sockaddr_in client_address;
    socklen_t address_lenght = sizeof( struct sockaddr_in );

    pthread_attr_t attr_thread;
    pthread_attr_init( &attr_thread );
    pthread_attr_setdetachstate( &attr_thread, PTHREAD_CREATE_DETACHED );

    struct sigaction *sig_action = ( struct sigaction * ) malloc( sizeof( struct sigaction ) );
    memset( sig_action, 0, sizeof( struct sigaction ) );
    
    sig_action->sa_flags = SA_RESTART;
    sig_action->sa_handler = send_game;
    sigaction( SIGUSR1, sig_action, NULL );
    sig_action->sa_handler = handler_new_player;
    sigaction( SIGUSR2, sig_action, NULL );
    signal( SIGALRM, sigalrm_h );
    signal( SIGTERM, docker_SIGTERM_h );
    free( sig_action );

    while( true )
    {
        if( ( client_sd = accept( sd, ( struct sockaddr * ) &client_address, &address_lenght ) ) < 0 )
        {
            perror( "Error accepted.\n" );
            continue;
        }

        pthread_t tid;

        if( pthread_create( &tid, &attr_thread, player_thread, &client_sd ) != 0 )
        {
            perror( "Error in the creation of the thread.\n" );
            continue;
        }
    }

    return 0;

}
