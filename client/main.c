#include "function.h"

char grid[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
int sd = 0;

//costanti per la disconnessione dalla partita
const char NOERROR = '0';
const char ERROR = '1';

//costanti di esito partita
const char NONE = '0';
const char WIN = '1';
const char LOSE = '2';
const char DRAW = '3';

int main()
{
    signal(SIGUSR1, SIGUSR1_handler);
    signal(SIGTERM, SIGTERM_handler);
    
    printf("Press ENTER to start...\n");
    fflush(stdout);
    getchar();

    init_socket();
    start_client_session();

    return 0;
}