#include "function.h"

/*	
    •	init_socket(): Inizializza una socket TCP, imposta timeout e si connette al server su 127.0.0.1:8080.
	•	start_client_session(): Avvia un thread per gestire l’input da tastiera e riceve messaggi dal server, attivando la partita quando richiesto.
	•	thread_writer(): Legge input da tastiera e lo invia al server; continua fino a quando l’utente scrive exit.
	•	play_games(char* inbuffer, enum player_type type): Contiene il ciclo di gioco: gestisce i turni, aggiorna la griglia e gestisce eventuali rematch.
	•	get_valid_move(): Chiede all’utente di inserire una mossa valida (1-9) oppure 0 per arrendersi.
	•	send_move(unsigned short int* moves): Invia la mossa dell’utente al server, aggiorna la griglia e controlla se la partita è finita.
	•	receive_move(unsigned short int* moves): Riceve e applica la mossa dell’avversario, controllando se la partita è terminata.
	•	rematch_host(): Gestisce la logica di rematch lato host: riceve l’intenzione dell’avversario e chiede conferma all’utente.
	•	rematch_opponent(): Gestisce il rematch lato opponent: accetta o rifiuta l’invito, poi attende decisione dell’host.
	•	check_outcome(const unsigned short int* moves): Verifica se c’è un vincitore o un pareggio analizzando righe, colonne e diagonali.
	•	check_move(int move): Verifica che la mossa sia tra 1 e 9 e che la cella scelta non sia già occupata.
	•	insertO(unsigned short int move): Inserisce il simbolo 'O' (mossa del giocatore) nella posizione indicata della griglia.
	•	insertX(unsigned short int move): Inserisce il simbolo 'X' (mossa dell’avversario) nella griglia.
	•	print_grid(): Stampa la griglia di gioco in un formato leggibile nel terminale.
	•	error_handler(): Gestisce errori: segnala disconnessione per inattività o chiude la socket in caso di errore.
	•	SIGUSR1_handler(): Termina il thread corrente (utilizzato per interrompere thread_writer).
	•	SIGTERM_handler(): Chiude la socket e termina il programma (usato per arresto pulito del client).
*/

// ===================================================================
// Funzione: init_socket
// Scopo: inizializza la connessione con il server tramite socket TCP
// ===================================================================

void init_socket()
{
    
    struct sockaddr_in server_addr; // Struttura per specificare l'indirizzo del server
    socklen_t addr_len = sizeof(server_addr); // Lunghezza della struttura server_addr

    sd = socket(AF_INET, SOCK_STREAM, 0); // Crea una socket TCP (SOCK_STREAM) su IPv4 (AF_INET)

    if (sd < 0) 
    {
        perror("Socket creation error."); // Messaggio di errore se la creazione fallisce
        exit(EXIT_FAILURE); // Interrompe il programma in caso di errore
    }

    struct timeval timeout = {
        .tv_sec = 300, // Timeout di 300 secondi (5 minuti)
        .tv_usec = 0 // Nessun tempo addizionale in microsecondi
    };
    
    // Imposta il timeout di invio
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) 
    {
        perror("Set socket timer error.");
        exit(EXIT_FAILURE);
    }

    // Imposta il timeout di ricezione
    if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) 
    {
        perror("Set socket timer error.");
        exit(EXIT_FAILURE);
    }

    const int opt = 1;
    // Disabilita il buffering TCP per inviare subito i pacchetti (Nagle's algorithm OFF)
    if (setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) 
    {
        perror("Tcp nodelay error.");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Inizializza la struttura server_addr a 0
    server_addr.sin_family = AF_INET; // Specifica IPv4
    server_addr.sin_port = htons(8080); // Imposta la porta a 8080 (convertita in network byte order)
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Imposta l'indirizzo IP a localhost

    // Connessione al server
    if (connect(sd, (struct sockaddr *)&server_addr, addr_len) < 0) 
    {
        perror("Server connection error.");
        exit(EXIT_FAILURE);
    }

}

// ================================================================
// Funzione: start_client_session
// Scopo: gestisce la sessione client, riceve dati e avvia partite
// ================================================================

void start_client_session()
{
    char inbuffer[MAXREADER]; // Buffer per leggere i messaggi dal server
    memset(inbuffer, 0, MAXREADER); // Pulisce il buffer

    pthread_t tid_writer; // Thread per gestire l'input da tastiera
    pthread_attr_t attr; // Attributi del thread
    pthread_attr_init(&attr); // Inizializzazione degli attributi
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // Il thread sarà staccato
    pthread_create(&tid_writer, &attr, thread_writer, NULL); // Creazione del thread writer

    printf("~TRIS-PROJECT~\n\n"); // Intestazione della sessione

    // Ciclo principale di ricezione dati dal server
    while (recv(sd, inbuffer, MAXREADER, 0) > 0)
    {
        if (strcmp(inbuffer, "Starting a game as host\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1); // Termina thread writer
            play_games(inbuffer, HOST); // Inizia partita come Host
            pthread_create(&tid_writer, &attr, thread_writer, NULL); // Ricrea thread writer
        }
        else if (strcmp(inbuffer, "Starting a game as opponent\n") == 0) 
        {
            pthread_kill(tid_writer, SIGUSR1); // Termina thread writer
            play_games(inbuffer, OPPONENT); // Inizia partita come Opponent
            pthread_create(&tid_writer, &attr, thread_writer, NULL); // Ricrea thread writer
        }
        else
        {
            printf("%s", inbuffer); // Stampa il messaggio ricevuto
        }
        memset(inbuffer, 0, MAXREADER); // Pulisce il buffer per la prossima ricezione
    }

    pthread_kill(tid_writer, SIGUSR1); // Termina thread writer se la connessione si chiude
    if (errno != 0)
    {
        error_handler(); // Gestisce eventuali errori
    }
    else
    {
        close(sd); // Chiude la socket se finisce normalmente
    }
}

// ====================================================
// Funzione: thread_writer
// Scopo: invia comandi digitati dall'utente al server
// ====================================================

void* thread_writer()
{
    char outbuffer[MAXWRITER]; // Buffer per l'output da inviare

    do
    {
        memset(outbuffer, 0, MAXWRITER); // Pulisce il buffer
        if (fgets(outbuffer, MAXWRITER, stdin) != NULL && outbuffer[strnlen(outbuffer, MAXWRITER)-1] == '\n')
        {
            if (outbuffer[0] != '\n') // Se l'input non è una riga vuota
            {
                if (send(sd, outbuffer, strlen(outbuffer)-1, 0) < 0)
                {
                    error_handler(); // Invia al server
                } 
            }
            else 
            {
                outbuffer[0] = '\0';
                printf("Please enter valid input\n"); // Avviso per input vuoto
            }
        }
        else
        {
            printf("You can write a maximum of 15 characters!\n"); // Input troppo lungo
            int c;
            while ((c = getchar()) != '\n' && c != EOF); // Svuota lo stdin
        }
    } while (strcmp(outbuffer, "exit\n") != 0); // Continua fino a "exit"

    close(sd); // Chiude la socket
    printf("Exit\n"); // Messaggio di uscita
    exit(EXIT_SUCCESS); // Uscita pulita del programma
}

// ================================================================
// Funzione: play_games
// Scopo: gestisce la logica di una o più partite, inclusi rematch
// ================================================================

void play_games(char *inbuffer, const enum player_type type)
{
    unsigned int round = 0; // Contatore dei round
    bool rematch = false; // Flag per richiedere rematch

    printf("\n~MATCH~\n");

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
        memset(grid, 0, 9); // Inizializza la griglia vuota
        round++; // Nuovo round

        unsigned short int moves = 0; // Contatore mosse
        char outcome = NONE; // Esito della partita
        char e_flag = NOERROR; // Flag per disconnessione avversario

        printf("\nRound %u\n", round);

        if ((type == OPPONENT && round%2 == 1) || (type == HOST && round%2 == 0))
        {
            printf("\nWaiting for the opponent...\n");
        }

        do
        {
            memset(inbuffer, 0, MAXREADER); // Pulisce il buffer

            // Il primo turno è del giocatore se inizia per il proprio tipo
            if (((type == HOST && round%2 == 1) || (type == OPPONENT && round%2 == 0)) && (moves == 0))
            {
                print_grid();
                printf("Your Turn\n");
                outcome = send_move(&moves);
                printf("Opponent Turn\n");
            }
            else
            {  
                if (recv(sd, &e_flag, 1, 0) <= 0)
                {
                    error_handler();    // Riceve segnale di errore o abbandono
                }  
                if (e_flag == ERROR) 
                {
                    outcome = WIN; // Vittoria automatica se avversario disconnesso
                    break;
                }
                if ((outcome = receive_move(&moves)) != NONE) break; // Riceve mossa avversario
                printf("Your Turn\n");
                if ((outcome = send_move(&moves)) != NONE) break; // Invia mossa
                printf("Opponent Turn\n");
            }
        } while (outcome == NONE);

        if (e_flag == ERROR) 
        {
            printf("Your opponent has disconnected, you have won the game by default.\n");
            break;
        }
        if (outcome != DRAW) break; // Se vittoria/sconfitta, termina

        // Gestione del rematch
        if (type == HOST)
        {
            rematch = rematch_host();
        } 
        else
        {
            rematch = rematch_opponent();
        } 

    } while(rematch); // Ripeti se rematch richiesto
}

// =======================================================================
// Funzione: get_valid_move
// Scopo: chiede all'utente una mossa valida tra 1 e 9 o 0 per arrendersi
// =======================================================================

int get_valid_move() 
{
    int c; // Carattere temporaneo per svuotare lo stdin
    char move[2] = {'\0', '\0'}; // Buffer per l'input dell'utente
    int move_index = 0; // Indice della mossa scelta
    bool valid_move = false; // Flag per verificare se la mossa è valida

    do {
        printf("Write a number from 1 to 9 to indicate where to place the symbol, or 0 to give up.\n");
        move[0] = getchar(); // Legge un carattere
        if (move[0] != '\n') 
        {
            move_index = atoi(move); // Converte la stringa in intero
            valid_move = check_move(move_index); // Verifica se la mossa è valida
            while ((c = getchar()) != '\n' && c != EOF); // Svuota il buffer di input
        }
        if (!valid_move)
        {
            printf("Invalid move\n"); // Messaggio di errore
        } 
    } while (!valid_move); // Ripeti finché la mossa non è valida

    return move_index; // Ritorna l'indice della mossa valida
}

// ======================================================================
// Funzione: send_move
// Scopo: invia la mossa dell'utente e restituisce l'esito della partita
// ======================================================================

char send_move(unsigned short int *moves)
{
    char outcome = NONE; // Esito iniziale della partita

    int move_index = get_valid_move(); // Ottiene una mossa valida dall'utente
    char move = move_index + '0'; // Converte la mossa in carattere

    send(sd, &move, 1, 0); // Invia la mossa al server

    if (move_index != 0)
    {
        insertO(move_index); // Inserisce il simbolo 'O' nella griglia
    }

    print_grid(); // Stampa la griglia aggiornata

    if (move_index == 0) // Caso in cui l'utente si arrende
    {
        printf("You gave up, the opponent wins\n");
        send(sd, &LOSE, 1, 0); // Invia al server il risultato "LOSE"
        return LOSE; // Restituisce LOSE
    }

    (*moves)++; // Incrementa il numero di mosse effettuate

    if (*(moves) >= 5)
    {
        outcome = check_outcome(moves); // Controlla l'esito della partita solo se ci sono almeno 5 mosse
    }

    send(sd, &outcome, 1, 0); // Invia l'esito della mossa al server
    return outcome; // Ritorna l'esito
}


// =============================================================
// Funzione: receive_move
// Scopo: riceve la mossa dell'avversario e aggiorna la griglia
// =============================================================

char receive_move(unsigned short int *moves)
{
    char move[2] = {'\0', '\0'}; // Buffer per ricevere la mossa
    char move_index = 0; // Indice della mossa ricevuta
    char outcome = NONE; // Esito della partita

    if (recv(sd, move, 1, 0) <= 0)
    {
        error_handler(); // Gestione errore in ricezione
    } 
    
    move_index = atoi(move); // Converte il carattere ricevuto in intero

    if (move_index != 0)
    {
        insertX(move_index); // Inserisce il simbolo 'X' nella griglia
    }

    print_grid(); // Stampa la griglia aggiornata

    if (move_index == 0) // L'avversario si è arreso
    {
        printf("The opponent has surrendered, you win\n");
        return WIN; // L'utente vince
    }

    (*moves)++; // Incrementa il numero di mosse

    outcome = check_outcome(moves); // Verifica se la partita è terminata
    return outcome; // Ritorna l'esito
}

// ==================================================================
// Funzione: rematch_host
// Scopo: gestisce la richiesta di rematch da parte del proprietario
// ==================================================================

bool rematch_host()
{
    char buffer[MAXREADER/2]; // Buffer per ricevere messaggi
    memset(buffer, 0, MAXREADER/2); // Inizializza il buffer a 0
    char reply; // Risposta dell'utente
    int c; // Carattere temporaneo per svuotare input
    bool valid_reply = false; // Flag per risposta valida

    printf("The opponent is choosing whether or not he wants a rematch..\n");

    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler(); // Errore nella ricezione della risposta
    } 
    printf("%s", buffer); // Mostra il messaggio ricevuto

    do
    {
        if (strcmp(buffer, "Rematch refused by opponent\n") == 0)
        {
            return false; // Se l'avversario ha rifiutato, termina
        } 
        else
        {
            memset(buffer, 0, MAXREADER/2); // Pulisce il buffer
        } 

        reply = getchar(); // Legge risposta dall'utente
        if (reply != '\n')
        {
            while ((c = getchar()) != '\n' && c != EOF); // Svuota stdin
            reply = toupper(reply); // Converte in maiuscolo
            if (reply != 'Y' && reply != 'N')
            {
                printf("Write a valid answer\n"); // Richiesta risposta valida
            } 
            else
            {
                valid_reply = true; // Risposta valida
            } 
        }
    } while(!valid_reply); // Ripeti finché non è valida

    send(sd, &reply, 1, 0 ); // Invia la risposta al server

    if (reply == 'N')
    {
        printf("Rematch refused\n");
        return false; // Nessun rematch
    }
    else
    {
        if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
        {
            error_handler(); // Errore nel ricevere conferma
        } 
        printf("%s", buffer); // Mostra conferma rematch
        return true; // Accetta rematch
    }
}

// =================================================================
// Funzione: rematch_opponent
// Scopo: gestisce la richiesta di rematch da parte dell'avversario
// =================================================================

bool rematch_opponent()
{
    char buffer[MAXREADER/2]; // Buffer per messaggi
    memset(buffer, 0, MAXREADER/2); // Pulisce il buffer
    char reply; // Risposta dell'utente
    int c; // Variabile per svuotare stdin
    bool valid_reply = false; // Flag per risposta valida

    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler(); // Gestisce errore di ricezione
    } 
    printf("%s", buffer); // Mostra il messaggio ricevuto

    do
    {
        memset(buffer, 0, MAXREADER/2); // Pulisce di nuovo il buffer

        reply = getchar(); // Legge la risposta da tastiera
        if (reply != '\n')
        {
            while ((c = getchar()) != '\n' && c != EOF); // Svuota input
            reply = toupper(reply); // Converte in maiuscolo
            if (reply != 'Y' && reply != 'N')
            {
                printf("Write a valid answer\n"); // Richiede risposta valida
            } 
            else
            {
                valid_reply = true; // Risposta valida
            } 
        }
    } while(!valid_reply); // Ripete finché non è valida

    send(sd, &reply, 1, 0 ); // Invia risposta al server

    if (reply == 'N')
    {
        printf("Rematch refused\n");
        return false; // Rifiuta rematch
    }

    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler(); // Errore ricezione
    } 
    printf("%s", buffer); // Mostra messaggio

    if (recv(sd, buffer, MAXREADER/2, 0) <= 0)
    {
        error_handler(); // Altro errore ricezione
    } 
    printf("%s", buffer); // Mostra messaggio

    if (strcmp(buffer, "Rematch refused by host\n") == 0)
    {
        return false; // Host ha rifiutato
    } 
    else
    {
        return true; // Rematch accettato
    } 
}


// =====================================================================================
// Funzione: check_outcome
// Scopo: controlla l'esito della partita in base allo stato della griglia e alle mosse
// =====================================================================================

char check_outcome(const unsigned short int *moves)
{
    char outcome = NONE; // Esito iniziale

    for (int i = 0; i < 3; i++) 
    {
        // Controlla righe
        if (grid[i][0] != 0 && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2])
        {
            return (grid[i][0] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
        }

        // Controlla colonne
        if (grid[0][i] != 0 && grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i])
        {
            return (grid[0][i] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
        }
    }

    // Controlla diagonale principale
    if (grid[0][0] != 0 && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2])
    {
        return (grid[0][0] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
    }

    // Controlla diagonale secondaria
    if (grid[0][2] != 0 && grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0])
    {
        return (grid[0][2] == 'O') ? (printf("You Won!\n"), WIN) : (printf("The opponent wins.\n"), LOSE);
    }

    // Se tutte le caselle sono piene e non c'è vincitore, è pareggio
    if (outcome == NONE && *moves == 9)
    {
        outcome = DRAW; 
        printf("It's a Draw!\n"); 
    }

    return outcome; // Ritorna l'esito finale
}

// ========================================================================
// Funzione: check_move
// Scopo: verifica se la mossa è valida (0 = resa, 1-9 = posizione libera)
// ========================================================================

bool check_move(const int move)
{
    if (move == 0)
    {
        return true; // La resa è sempre valida
    } 
    if (move < 0 || move > 9)
    {
        return false; // Mossa fuori dai limiti
    }

    const unsigned short int column_index = (move - 1) % 3; // Colonna della griglia
    const unsigned short int row_index = (move - 1) / 3; // Riga della griglia

    if (grid[row_index][column_index] != '\0')
    {
        return false; // Casella già occupata
    } 
    else
    {
        return true; // Casella libera
    } 
}

// =================================================================
// Funzione: insertO
// Scopo: inserisce il simbolo 'O' nella griglia in base alla mossa
// =================================================================

void insertO(const unsigned short int move)
{
    const unsigned short int column_index = (move - 1) % 3; // Calcolo colonna
    const unsigned short int row_index = (move - 1) / 3; // Calcolo riga
    grid[row_index][column_index] = 'O'; // Inserisce 'O' nella cella corrispondente
}

// =================================================================
// Funzione: insertX
// Scopo: inserisce il simbolo 'X' nella griglia in base alla mossa
// =================================================================

void insertX(const unsigned short int move)
{
    const unsigned short int column_index = (move - 1) % 3; // Calcola l'indice della colonna
    const unsigned short int row_index = (move - 1) / 3; // Calcola l'indice della riga
    grid[row_index][column_index] = 'X'; // Inserisce 'X' nella posizione specificata
}


// ================================================================
// Funzione: print_grid
// Scopo: stampa a schermo lo stato attuale della griglia di gioco
// ================================================================

void print_grid()
{
    printf("\n\n");

    for (int i = 0; i < 3; i++) {
        printf("|"); // Inizia una nuova riga
        for (int j = 0; j < 3; j++) {
            char c = (grid[i][j] == '\0') ? ' ' : grid[i][j]; // Se cella vuota stampa spazio, altrimenti simbolo
            printf("   %c   |", c); // Stampa cella con bordi
        }
        printf("\n"); // Fine riga
    }

    printf("\n\n");
}

// ==============================================================
// Funzione: error_handler
// Scopo: gestisce errori durante la comunicazione con il server
// ==============================================================

void error_handler()
{
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        printf("Inactivity disconnection\n"); // Timeout per inattività
    } 
    else 
    {   
        printf("An error occurred\n"); // Errore generico
        close(sd); // Chiude la socket
    }
    exit(EXIT_FAILURE); // Termina il programma con codice di errore
}

// ===============================================================
// Funzione: SIGUSR1_handler
// Scopo: gestore del segnale SIGUSR1, termina il thread corrente
// ===============================================================

void SIGUSR1_handler()
{
    pthread_exit(NULL); // Termina il thread senza causare side-effect
}

// ============================================================================
// Funzione: SIGTERM_handler
// Scopo: gestore del segnale SIGTERM, chiude la socket e termina il programma
// ============================================================================

void SIGTERM_handler()
{
    close(sd); // Chiude la socket in uso
    exit(EXIT_SUCCESS); // Termina il programma con successo
}