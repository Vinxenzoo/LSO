#include "function.h"

void init_socket()
{
    /*
    struct sockaddr_in ser_add;
    socklen_t lenght = sizeof(struct sockaddr_in);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation error"), exit(EXIT_FAILURE);
    }   

    const int opt = 1;
    struct timeval timer;
    timer.tv_sec = 300;  // Timer di 300 secondi
    timer.tv_usec = 0;

    //timer per invio e ricezione
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timer, sizeof(timer)) < 0 || setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) < 0)
    {
        perror("set socket timer error"), exit(EXIT_FAILURE);
    }
        
    //disattiva bufferizzazione socket
    if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
    {
        perror("tcp nodelay error"), exit(EXIT_FAILURE);
    }

    memset(&ser_add, 0, sizeof(ser_add));
    ser_add.sin_family = AF_INET;
    ser_add.sin_port = htons(8080);
    ser_add.sin_addr.s_addr = inet_addr("127.0.0.1");

    //non c'è bisogno di inizializzare manualmente la porte client e fare il bind

    if (connect(sd, (struct sockaddr *)&ser_add, lenght) < 0)
    {
        perror("connect error"), exit(EXIT_FAILURE);
    }
    */
    
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Crea socket TCP
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) 
    {
        perror("Errore nella creazione della socket");
        exit(EXIT_FAILURE);
    }

    // Imposta timeout di invio/ricezione a 300 secondi
    struct timeval timeout = {
        .tv_sec = 300,
        .tv_usec = 0
    };
    
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) 
    {
        perror("Errore nel timeout di invio");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) 
    {
        perror("Errore nel timeout di ricezione");
        exit(EXIT_FAILURE);
    }

    // Disabilita Nagle Algorithm (bufferizzazione TCP)
    const int opt = 1;
    if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) 
    {
        perror("Errore nell'impostazione TCP_NODELAY");
        exit(EXIT_FAILURE);
    }

    // Inizializza indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connessione al server
    if (connect(sd, (struct sockaddr *)&server_addr, addr_len) < 0) 
    {
        perror("Errore di connessione al server");
        exit(EXIT_FAILURE);
    }
    
}

void start_client_session()
{
    /*
    char inbuffer[MAXREADER];
    memset(inbuffer, 0, MAXREADER);

    //il thread principale crea un thread scrittore detatched
    pthread_t tid_writer = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_writer, &attr, thread_writer, NULL);

    printf("~TRIS-LSO~\n");
    //il thread scrittore viene ucciso per giocare le partite per evitare conflitti con le send
    while (recv(sd, inbuffer, MAXREADER, 0) > 0)
    {
        if (strcmp(inbuffer, "*** Start the game as host ***\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1);
            play_games(inbuffer, HOST);
            pthread_create(&tid_writer, &attr, thread_writer, NULL);
        }
        else if (strcmp(inbuffer, "*** Start the game as opponent ***\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1);
            play_games(inbuffer, OPPONENT);
            pthread_create(&tid_writer, &attr, thread_writer, NULL);
        }
        else printf("%s", inbuffer);
        memset(inbuffer, 0, MAXREADER);
    }
    if (errno != 0)
    {
        pthread_kill(tid_writer, SIGUSR1);
        error_handler();
    }
    else
    {
        pthread_kill(tid_writer, SIGUSR1);
        close(sd);
    }
    */

    
    char inbuffer[MAXREADER];
    memset(inbuffer, 0, MAXREADER);

    // Inizializzazione e creazione del thread scrittore in modalità detached
    pthread_t tid_writer;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_writer, &attr, thread_writer, NULL);

    // Header iniziale
    printf("~TRIS-LSO~\n");

    // Loop di ricezione dei messaggi dal server
    while (recv(sd, inbuffer, MAXREADER, 0) > 0)
    {
        if (strcmp(inbuffer, "~Host~\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1); // Ferma il thread di scrittura
            play_games(inbuffer, HOST);        // Inizia partita da host
            pthread_create(&tid_writer, &attr, thread_writer, NULL); // Ricrea il thread dopo la partita
        }
        else if (strcmp(inbuffer, "\n~Opponent~\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1); // Ferma il thread di scrittura
            play_games(inbuffer, OPPONENT);    // Inizia partita da ospite
            pthread_create(&tid_writer, &attr, thread_writer, NULL); // Ricrea il thread dopo la partita
        }
        else
            printf("%s", inbuffer); // Stampa normali messaggi dal server
            memset(inbuffer, 0, MAXREADER); // Pulisce il buffer per il prossimo ciclo
    }

    // Gestione della chiusura della connessione
    pthread_kill(tid_writer, SIGUSR1); // Assicura la terminazione del writer
    if (errno != 0)
    {
        error_handler();
    }
    else
    {
        close(sd);
    }
    
}

void* thread_writer()
{
    char outbuffer[MAXWRITER];

    do
    {
        memset(outbuffer, 0, MAXWRITER);
        //strnlen è più sicura di strlen per stringhe che potrebbero non terminare con \0 come in questo caso
        if (fgets(outbuffer, MAXWRITER, stdin) != NULL && outbuffer[strnlen(outbuffer, MAXWRITER)-1] == '\n') //massimo 15 caratteri nel buffer escluso \n
        {
            if (outbuffer[0] != '\n') //input valido
            {
                if (send(sd, outbuffer, strlen(outbuffer)-1, 0) < 0) error_handler();
            }
            else 
            {
                outbuffer[0] = '\0'; //input vuoto
                printf("Please enter valid input\n");
            }
        }
        else //sono stati scritti più di 15 caratteri
        {
            printf("You can write a maximum of 15 characters!\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF); //svuota lo standard input (fflush non funziona)
        }
    } while (strcmp(outbuffer, "exit\n") != 0);
    close(sd);
    printf("Exit\n");
    exit(EXIT_SUCCESS);
}


void play_games(char *inbuffer, const enum player_type type)
{

    unsigned int round = 0;
    bool rematch = false;
    
    if (type == HOST) 
    {
        printf("\nRequest accepted, the game has started!\n");
    }
    else 
    {
        printf("\nThe host has accepted your request, the game has started!\n");
    }

    do
    {
        memset(grid, 0, 9); //ad ogni iterazione la griglia viene pulita
        round++;

        unsigned short int moves = 0;
        char outcome = NONE;
        char e_flag = NOERROR; //il server manda ERROR in caso di errore

        printf("\nRound %u\n", round);

        //il proprietario comincia per primo nei turni dispari e viceversa
        if ((type == OPPONENT && round%2 == 1) || (type == HOST && round%2 == 0))
        {
            printf("\nWaiting for the opponent..\n");
        }

        do
        {
            memset(inbuffer, 0, MAXREADER);

            //controllo che serve a distinguere chi comincia per primo
            if (((type == HOST && round%2 == 1) || (type == OPPONENT && round%2 == 0)) && (moves == 0))
            {   //ogni player vede egli stesso come O e l'avversario come X
                print_grid();
                printf("Your Turn\n");
                outcome = send_move(&moves);
                printf("Opponent Turn\n");
            }
            else //dal secondo turno in poi deve prima ricevere la giocata dell'avversario e poi iniziare il suo turno
            {    //viene controllato il flag di errore prima di ricevere la giocata dell'avversario
                if (recv(sd, &e_flag, 1, 0) <= 0) error_handler(); 
                if (e_flag == ERROR) 
                {
                    outcome = WIN; 
                    break;
                }
                //non c'è errore, riceve la giocata dell'avversario
                if ((outcome = receive_move(&moves)) != NONE) break;
                printf("Your Turn\n");
                if ((outcome = send_move(&moves)) != NONE) break;
                printf("Opponent Turn\n");
            }
        } while (outcome == NONE);

        if (e_flag == ERROR) 
        {
            printf("Your opponent has disconnected, you have won the game by default.\n"); break;
        }
        if (outcome != DRAW) break;

        //eventuale rivincita in caso di pareggio
        if (type == HOST)
        {
            rematch = rematch_host();
        } 
        else
        {
            rematch = rematch_opponent();
        } 

    }while(rematch);
}

int get_valid_move() 
{
    int c;
    char move[2] = {'\0', '\0'};
    int move_index = 0;
    bool valid_move = false;

    do {
        printf("Write a number from 1 to 9 to indicate where to place the symbol, or 0 to give up.\n");
        move[0] = getchar();
        if (move[0] != '\n') {
            move_index = atoi(move);
            valid_move = check_move(move_index);
            while ((c = getchar()) != '\n' && c != EOF);
        }
        if (!valid_move) printf("Invalid bet\n");
    } while (!valid_move);

    return move_index;
}

char send_move(unsigned short int *moves)
{
    char outcome = NONE;

    //ottiene una mossa valida (da 1 a 9 o 0 per resa)
    int move_index = get_valid_move();
    char move = move_index + '0';

    // invia la mossa al server
    send(sd, &move, 1, 0);

    //inserisce la giocata nella propria griglia locale e invia l'esito al server
    if (move_index != 0)
    {
        insertO(move_index);
    }

    //stampa la griglia dopo averla pulita
    if (system("clear") < 0)
    {
        perror("Error"), exit(EXIT_FAILURE);
    }
    print_grid();

    //in caso di resa
    if (move_index == 0)
    {
        printf("You gave up, the opponent wins\n");
        send(sd, &LOSE, 1, 0);
        return LOSE;
    }

    (*moves)++;

    //calcola esito se sono state fatte almeno 5 mosse
    if (*(moves) >= 5)
    {
        outcome = check_outcome(moves);
    }

    //invia esito al server
    send(sd, &outcome, 1, 0);
    return outcome;
}

char receive_move(unsigned short int *moves)
{
    char move[2] = {'\0', '\0'};
    char move_index = 0;
    char outcome = NONE;

    //da per scontato che la giocata sia valida
    if (recv(sd, move, 1, 0) <= 0)
    {
        error_handler();
    } 
    move_index = atoi(move);

    if (move_index != 0)
    {
        insertX(move_index);
    } 

    if (system("clear") < 0)
    {
        perror("Error"), exit(EXIT_FAILURE);
    } 

    print_grid();

    if (move_index == 0)
    {
        printf("The opponent has surrendered, you win\n");
        return WIN;
    }
    (*moves)++;


    outcome = check_outcome(moves);
    //non invia l'esito perchè se ne occupa chi invia la giocata
    return outcome; 
}

bool rematch_host()
{
    char buffer[MAXREADER/2];
    memset(buffer, 0, MAXREADER/2);
    char reply;
    int c;
    bool valid_reply = false;

    printf("The opponent is choosing whether or not he wants a rematch..\n");

    //Il proprietario riceve risposta dell'avversario
    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler();
    } 
    printf("%s", buffer);

    do //verifica che l'input sia valido
    {
        if (strcmp(buffer, "Rematch refused by opponent\n") == 0)
        {
            return false;
        } 
        else
        {
            memset(buffer, 0, MAXREADER/2);
        } 

        reply = getchar();
        if (reply != '\n') //il giocatore ha premuto invio senza dare input
        {
            while ((c = getchar()) != '\n' && c != EOF);  // Svuota lo stdin per sicurezza
            reply = toupper(reply);
            if (reply != 'Y' && reply != 'N')
            {
                printf("Write a valid answer\n"); 
            } 
            else
            {
                valid_reply = true; 
            } 
        }
    } while(!valid_reply);

    //input corretto, lo invia al server
    send(sd, &reply, 1, 0 );

    if (reply == 'N')
    {
        printf("Rematch refused\n");
        return false;
    }
    else //il server invia il messaggio di conferma rivincita
    {
        if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
        {
            error_handler();
        } 
        printf("%s", buffer);
        return true;
    }
}

bool rematch_opponent()
{
    char buffer[MAXREADER/2];
    memset(buffer, 0, MAXREADER/2);
    char reply;
    int c;
    bool valid_reply = false;

    //Riceve dal server richiesta di rivincita
    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler();
    } 
    printf("%s", buffer);

    do //verifica che l'input sia valido
    {
        memset(buffer, 0, MAXREADER/2);

        reply = getchar();
        if (reply != '\n') //il giocatore ha premuto invio senza dare input
        {
            while ((c = getchar()) != '\n' && c != EOF);  // Svuota lo stdin per sicurezza
            reply = toupper(reply);
            if (reply != 'Y' && reply != 'N')
            {
                printf("Write a valid answer\n"); 
            } 
            else
            {
                valid_reply = true;
            } 
        }
    } while(!valid_reply);

    //input corretto, lo invia al server
    send(sd, &reply, 1, 0 );

    if (reply == 'N')
    {
        printf("Rematch refused\n");
        return false;
    }

    //all'avversario viene chiesto di attendere il proprietario (se ha scelto la rivincita)
    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler();
    } 
    printf("%s", buffer);

    //risposta del proprietario
    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler();
    } 
    printf("%s", buffer);

    //proprietario rifiuta
    if (strcmp(buffer, "Rematch refused by host\n") == 0)
    {
        return false;
    } 
    else
    {
        return true;
    } 
}

char check_outcome(const unsigned short int *moves)
{
    /*
    char outcome = NONE;
    //se la funzione trova un tris di O restituisce 1, tris di X restituisce 2, pareggio restituisce 3
    // Controlla le righe
    for (int i = 0; i < 3; i++) 
    {
        if (grid[i][0] != 0 && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2])
        {
            if (grid[i][0] == 'O')
            {
                outcome = WIN; 
                printf("You Won!\n"); 
            }
            else
            { 
                outcome = LOSE;
                printf("The opponent wins.\n"); 
            }
        }
    }
    // Controlla le colonne
    for (int i = 0; i < 3; i++) 
    {
        if (grid[0][i] != 0 && grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i])
        {
            if (grid[0][i] == 'O')
            {
                outcome = WIN; 
                printf("You Won!\n"); 
            }
            else
            { 
                outcome = LOSE; 
                printf("The opponent wins.\n"); 
            }
        }
    }

    // Controlla la diagonale principale
    if (grid[0][0] != 0 && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2])
    {
        if (grid[0][0] == 'O')
        {
            outcome = WIN; 
            printf("You Won!\n"); 
        }
        else
        {
            outcome = LOSE; 
            printf("The opponent wins.\n"); 
        }
    }

    // Controlla la diagonale secondaria
    if (grid [0][2] != 0 && grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0])
    {
        if(grid[0][2] == 'O')
        {
            outcome = WIN; 
            printf("You Won!\n"); 
        }
        else
        { 
            outcome = LOSE;
            printf("The opponent wins.\n"); 
        }
    }
    */
   
    char outcome = NONE;

    //Controlla righe e colonne
    for (int i = 0; i < 3; i++) 
    {
        //Righe
        if (grid[i][0] != 0 && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2])
        {
            return (grid[i][0] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
        }

        //Colonne
        if (grid[0][i] != 0 && grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i])
        {
            return (grid[0][i] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
        }
            
    }

    //Diagonale principale
    if (grid[0][0] != 0 && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2])
    {
        return (grid[0][0] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
    }

    //Diagonale secondaria
    if (grid[0][2] != 0 && grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0])
    {
        return (grid[0][2] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
    }
    
    //Pareggio 
    if (outcome == NONE && *moves == 9)
    {
        outcome = DRAW; 
        printf("It's a Draw!\n"); 
    }

    return outcome;
}

bool check_move(const int move)
{
    if (move == 0)
    {
        return true;
    } 
    if (move < 0 || move > 9)
    {
        return false;
    }

    //controlla se la casella selezionata è gia occupata
    const unsigned short int column_index = (move - 1) % 3;
    const unsigned short int row_index = (move - 1) / 3;

    if (grid[row_index][column_index] != '\0')
    {
        return false;
    } 
    else
    {
        return true;
    } 
}

void insertO(const unsigned short int move)
{
    const unsigned short int column_index = (move - 1) % 3;
    const unsigned short int row_index = (move - 1) / 3;
    grid[row_index][column_index] = 'O';
}

void insertX(const unsigned short int move)
{
    const unsigned short int column_index = (move - 1) % 3;
    const unsigned short int row_index = (move - 1) / 3;
    grid[row_index][column_index] = 'X';
}

void print_grid()
{
    printf("\n\n");

    for (int i = 0; i < 3; i++) {
        printf("|");
        for (int j = 0; j < 3; j++) {
            char c = (grid[i][j] == '\0') ? ' ' : grid[i][j];
            printf("   %c   |", c);
        }
        printf("\n");
    }

    printf("\n\n");
}

void error_handler()
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        printf("Inactivity disconnection\n");
    } 
    else 
    {   
        printf("An error occurred\n");
        close(sd);
    }
    exit(EXIT_FAILURE);
}

void SIGUSR1_handler()
{
    //viene mandato dal thread principale per uccidere il thread scrittore
    pthread_exit(NULL);
}

void SIGTERM_handler()
{
    close(sd);
    exit(EXIT_SUCCESS);
}