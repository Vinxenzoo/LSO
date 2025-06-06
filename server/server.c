#include "server.h"

// ================================================================================================================
// Funzione: init_server
// Scopo: Inizializza una socket TCP per accettare connessioni in ingresso sulla porta 8080 su tutte le interfacce
// ================================================================================================================

int init_server()
{
    int sd; // Socket descriptor restituito dalla funzione
    int opt = 1; // Opzione per riutilizzare la porta
    struct sockaddr_in address; // Struttura che rappresenta l'indirizzo del server
    socklen_t lenght = sizeof(struct sockaddr_in); // Dimensione della struttura address

    // Crea una socket IPv4 (AF_INET) di tipo TCP (SOCK_STREAM)
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error"); // Errore nella creazione della socket
        exit(EXIT_FAILURE);
    }

    // Imposta l'opzione SO_REUSEADDR per permettere il riutilizzo dell'indirizzo
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        perror("Error setsockopt"); // Errore nella configurazione della socket
        close(sd); // Chiude la socket prima di uscire
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, lenght); // Azzera la struttura address
    address.sin_family = AF_INET; // Imposta il tipo di indirizzo a IPv4
    address.sin_port = htons(8080); // Imposta la porta in network byte order
    address.sin_addr.s_addr = INADDR_ANY; // Accetta connessioni da qualsiasi indirizzo IP

    // Associa la socket all'indirizzo specificato
    if (bind(sd, (struct sockaddr *) &address, lenght) < 0)
    {
        perror("Binding error"); // Errore durante il bind
        exit(EXIT_FAILURE);
    }

    // Mette la socket in ascolto con una coda di massimo 5 connessioni pendenti
    if (listen(sd, 5) < 0)
    {
        perror("Listen error"); // Errore nella messa in ascolto
        exit(EXIT_FAILURE);
    }

    return sd; // Restituisce il socket descriptor pronto per l'uso
}

// ==========================================================================================
// Funzione: player_thread
// Scopo: Gestisce un client accettato creando la struttura giocatore e avviando la sessione
// ==========================================================================================

void* player_thread(void *sd)
{
    const int player_sd = *((int *) sd); // Converte il puntatore generico in intero (socket client)

    struct PlayerNode *player = (struct PlayerNode *) malloc(sizeof(struct PlayerNode)); // Alloca memoria per il nuovo giocatore

    player = init_player(player_sd); // Inizializza la struttura con i dati del giocatore connesso

    handle_player_session(player, player_sd); // Avvia la gestione della sessione del giocatore

    pthread_exit(NULL); // Termina il thread in modo sicuro
}

// ==============================================================================================================
// Funzione: handle_player_session
// Scopo: Mostra il nuovo giocatore, lo inserisce nella lobby, chiude la socket e rimuove il giocatore alla fine
// ==============================================================================================================

void* handle_player_session(struct PlayerNode *player, const int player_sd)
{
    show_new_player(); // Mostra nel terminale la presenza di un nuovo giocatore
    lobby_handler(player); // Gestisce la logica della lobby per il giocatore
    close(player_sd); // Chiude la connessione con il client
    delete_player(player); // Rimuove il giocatore dalla lista e libera la memoria
}

// ====================================================================================================================
// Funzione: init_player
// Scopo: Riceve e assegna il nome del giocatore e inizializza il nodo del giocatore, aggiungendolo alla lista globale
// ====================================================================================================================

struct PlayerNode* init_player(const int player_sd)
{
    char *player_name = (char *) malloc(MAX_NAME_PLAYER * sizeof(char)); // Alloca buffer per il nome del giocatore

    // Invio messaggio di benvenuto e richiesta nome
    if (send(player_sd, "Enter your name to register(max 15 characters)\n", 47, MSG_NOSIGNAL) < 0)
    {
        close(player_sd); // Chiude connessione
        free(player_name); // Libera memoria del nome
        pthread_exit(NULL); // Esce dal thread
    }

    check_player_name(player_name, player_sd); // Verifica se il nome è disponibile

    struct PlayerNode *player = (struct PlayerNode *) malloc(sizeof(struct PlayerNode)); // Alloca struttura giocatore
    memset(player, 0, sizeof(struct PlayerNode)); // Pulisce la memoria

    // Inizializzazione dei campi
    strcpy(player->name, player_name);
    player->wins = 0;
    player->losts = 0;
    player->draws = 0;
    pthread_mutex_init(&(player->mutex_state), NULL);
    pthread_cond_init(&(player->cv_state), NULL);
    player->status = IN_LOBBY;
    player->player_tid = pthread_self();
    player->player_sd = player_sd;
    player->champion = false;

    // Inserisce il nodo in testa alla lista globale protetta
    pthread_mutex_lock(&player_mutex);
    player->next_node = player_head;
    player_head = player;
    pthread_mutex_unlock(&player_mutex);

    free(player_name); // Libera il buffer temporaneo
    return player; // Ritorna il nodo inizializzato
}

// ==============================================================================================================
// Funzione: check_player_name
// Scopo: Riceve e controlla che il nome scelto dal giocatore sia unico, altrimenti chiede di inserirne un altro
// ==============================================================================================================

char* check_player_name(char *player_name, const int player_sd)
{
    bool name_found = false; // Flag per sapere se il nome è valido

    do
    {
        memset(player_name, 0, MAX_NAME_PLAYER); // Pulisce buffer nome
        recv(player_sd, player_name, MAX_NAME_PLAYER, 0); // Riceve nome dal client

        if (!player_found(player_name)) // Se il nome non è già usato
        {
            name_found = true;
        }
        else if (send(player_sd, "Name already taken, choose another name\n", 40, MSG_NOSIGNAL) < 0)
        {
            close(player_sd);
            free(player_name);
            pthread_exit(NULL);
        }

    } while (!name_found); // Continua finché il nome è già occupato

    // Conferma avvenuta registrazione
    if (send(player_sd, "Registration Completed!\n", 24, MSG_NOSIGNAL) < 0)
    {
        close(player_sd);
        free(player_name);
        printf("Check failed.\n");
        pthread_exit(NULL);
    }

    return player_name; // Restituisce il nome valido
}

// ============================================================================
// Funzione: player_found
// Scopo: Controlla se un nome è già presente nella lista dei giocatori attivi
// ============================================================================

bool player_found(const char *player_name)
{
    pthread_mutex_lock(&player_mutex); // Protegge accesso alla lista
    struct PlayerNode *player = player_head;

    while (player != NULL)
    {
        if (strcmp(player->name, player_name) == 0) // Confronto nomi
        {
            pthread_mutex_unlock(&player_mutex);
            return true;
        }
        player = player->next_node;
    }

    pthread_mutex_unlock(&player_mutex); // Fine accesso concorrente
    return false; // Nome non trovato
}

// ====================================================================================
// Funzione: player_lobby
// Scopo: Notifica a tutti gli altri client in lobby che è presente un nuovo giocatore
// ====================================================================================

void player_lobby()
{
    // Sezione critica: acquisisce il mutex per accedere in sicurezza alla lista dei giocatori
    pthread_mutex_lock(&player_mutex); 
    
    // Ottiene il puntatore al primo nodo della lista dei giocatori
    struct PlayerNode *player = player_head;

    // Rilascia il mutex dopo aver ottenuto il puntatore alla testa della lista
    pthread_mutex_unlock(&player_mutex);

    // Ottiene l'ID del thread corrente (il chiamante della funzione)
    const pthread_t sender_tid = pthread_self(); 

    // Variabile temporanea per contenere l'ID dei thread dei destinatari del segnale
    pthread_t receiver_tid = 0;

    // Itera su tutta la lista dei giocatori attualmente registrati
    while (player != NULL)
    {
        // Ottiene l'ID del thread associato al giocatore corrente
        receiver_tid = player->player_tid;

        // Verifica che:
        // 1. Il thread del destinatario non sia quello del chiamante (evita di notificare se stesso)
        // 2. Il giocatore sia attualmente nello stato IN_LOBBY (solo i presenti in lobby devono ricevere notifica)
        if (receiver_tid != sender_tid && player->status == IN_LOBBY)
        {
            // Invia il segnale SIGUSR2 al thread del giocatore, notificandogli che un nuovo giocatore è entrato nella lobby.
            pthread_kill(receiver_tid, SIGUSR2); 
        }

        // Passa al nodo successivo nella lista dei giocatori
        player = player->next_node; 
    }
}

// ==========================================================================================================================
// Funzione: lobby_handler
// Scopo: Gestisce l’interazione dell’utente nella lobby, ovvero: visualizzazione statistiche, creazione o accesso a partite
// ==========================================================================================================================

void lobby_handler(struct PlayerNode *player)
{
    char inbuffer[MAX_MESSAGE_SIZE];   // Buffer per ricevere input dal client (comandi)
    char outbuffer[MAX_MESSAGE_SIZE];  // Buffer per inviare messaggi al client

    const int sock_desc_player = player->player_sd; // Socket descriptor associato al giocatore
    bool connected = true; // Flag che determina se il ciclo della lobby deve continuare

    do
    {
        // Inizializza i buffer a 0 per evitare dati sporchi da precedenti cicli
        memset(inbuffer, 0, MAX_MESSAGE_SIZE);
        memset(outbuffer, 0, MAX_MESSAGE_SIZE);

        player->status = IN_LOBBY; // Imposta lo stato iniziale del giocatore come "in lobby"

        // Converte le statistiche del giocatore (vittorie, sconfitte, pareggi) in stringhe
        char vittorie[3];   sprintf(vittorie, "%u", player->wins);
        char sconfitte[3];  sprintf(sconfitte, "%u", player->losts);
        char pareggi[3];    sprintf(pareggi, "%u", player->draws);

        // Costruisce il messaggio informativo con lo storico delle partite
        outbuffer[0] = '\n';
        strcat(outbuffer, "~HISTORY~\n");
        strcat(outbuffer, "\nWins: ");    strcat(outbuffer, vittorie);
        strcat(outbuffer, "\nLosts: ");   strcat(outbuffer, sconfitte);
        strcat(outbuffer, "\nDraws: ");   strcat(outbuffer, pareggi);

        // Invia le statistiche al client
        if (send(sock_desc_player, outbuffer, strlen(outbuffer), MSG_NOSIGNAL) < 0)
        {
            err_handler(sock_desc_player);
        }

        send_game(); // Invoca la funzione che invia la lista aggiornata delle partite disponibili

        do
        {
            if (errno == EINTR) errno = 0; // Ignora eventuali segnali di interruzione
            // Attende l’input del client. Se la ricezione fallisce e non è dovuta a un segnale, gestisce errore.
            if (recv(sock_desc_player, inbuffer, MAX_MESSAGE_SIZE, 0) <= 0 && errno != EINTR)
            {
                err_handler(sock_desc_player);
            }   
        } while (errno == EINTR); // Se il segnale ha interrotto `recv`, riprova

        // Converte tutti i caratteri del comando ricevuto in maiuscolo per uniformare il confronto
        int i = 0;
        while (inbuffer[i] != '\0')
        {
            inbuffer[i] = toupper(inbuffer[i]);
            i++;
        }

        // Se il comando ricevuto è "ESC", si esce dalla lobby e termina il ciclo
        if (strcmp(inbuffer, "ESC") == 0)
        {
            connected = false; // Disconnessione
            break;
        }
        // Se il comando è "CREATE", crea una nuova partita
        else if (strcmp(inbuffer, "CREATE") == 0)
        {
            do 
            {
                struct GameNode *game_node = new_game(player->name, sock_desc_player); // Crea nuova partita
                manage_game(game_node, sock_desc_player); // Gestisce la nuova partita
            } while (player->champion && !quit(sock_desc_player)); // Finché è campione e vuole accettare altri giocatori

            // Dopo il ciclo, ritorna in lobby
            if (send(sock_desc_player, "Back to the lobby\n", 18, MSG_NOSIGNAL) < 0)
            {
                err_handler(sock_desc_player);
            }
        }
        else // Altrimenti si assume che il comando sia un indice per un tentativo di join a una partita
        {
            int index = atoi(inbuffer); // Converte l’input in numero intero (ID partita)
            struct GameNode *match = NULL;

            if (index != 0)
            {
                match = find_game_by_index(index); // Cerca la partita corrispondente all’indice
            }

            // Se non trova la partita, notifica errore al client
            if (match == NULL)
            {
                if (send(sock_desc_player, "Match not found\n", 16, MSG_NOSIGNAL) < 0)
                {
                    err_handler(sock_desc_player);
                }
            }
            else // Se la partita è valida...
            {
                player->status = REQUESTING; // Stato del giocatore impostato su "richiesta di accesso"

                // Se il proprietario rifiuta o la richiesta fallisce
                if (!match_accepted(match, sock_desc_player, player->name))
                {
                    if (match->join_request == false)
                    {
                        if (send(sock_desc_player, "Join request refused\n", 21, MSG_NOSIGNAL) < 0)
                        {
                            err_handler(sock_desc_player);
                        }
                    }
                    else // Se la partita è già occupata da un altro avversario
                    {
                        if (send(sock_desc_player, "Another player have already take place in game\n", 47, MSG_NOSIGNAL) < 0)
                        {
                            err_handler(sock_desc_player);
                        }
                    }
                }
                else // La richiesta è stata accettata: inizia la partita
                {
                    // Attende la fine della partita con una wait sul proprio mutex
                    pthread_mutex_lock(&(player->mutex_state));
                    player->status = IN_GAME;

                    while (player->status == IN_GAME)
                    {
                        pthread_cond_wait(&(player->cv_state), &(player->mutex_state));
                    }

                    pthread_mutex_unlock(&(player->mutex_state));

                    // Se il giocatore è vincitore e desidera accettare un altro sfidante
                    while (player->champion && !quit(sock_desc_player))
                    {
                        struct GameNode *game_node = new_game(player->name, sock_desc_player);
                        manage_game(game_node, sock_desc_player);
                    }

                    // Al termine della sequenza di partite, ritorna in lobby
                    if (send(sock_desc_player, "Back to the lobby\n", 18, MSG_NOSIGNAL) < 0)
                    {
                        err_handler(sock_desc_player);
                    }
                }
            }
        }
    } while (connected);
}

// =================================================================
// Funzione: manage_game
// Scopo: Gestisce l’avvio e la terminazione di una singola partita
// =================================================================

void* manage_game(struct GameNode* game_node, const int sock_desc_player)
{
    if (game_node != NULL)
    {
        play_game(game_node); // Avvia partita
        delete_game(game_node); // Elimina struttura partita al termine
    }
    else
    {
        if (send(sock_desc_player, "Impossible to create a game, wait a moment...\n", 46, MSG_NOSIGNAL) < 0)
        {
            err_handler(sock_desc_player);
        }
    }
}

// =========================================================
// Funzione: new_game
// Scopo: Alloca e inizializza una nuova struttura GameNode
// =========================================================

struct GameNode* new_game(const char *owner_name, const int owner_sd)
{
    struct GameNode *game = (struct GameNode *) malloc(sizeof(struct GameNode)); // Alloca memoria per la nuova partita

    if (game == NULL) // Controlla esito allocazione
    {
        return NULL; // Errore nella creazione
    }

    init_game(game, owner_name, owner_sd); // Inizializza i campi della partita

    return game; // Ritorna la partita creata
}

// =======================================================================
// Funzione: init_game
// Scopo: Imposta i valori iniziali della partita e la inserisce in lista
// =======================================================================

void* init_game(struct GameNode* game, const char *owner_name, const int owner_sd)
{
    memset(game, 0, sizeof(struct GameNode)); // Pulisce tutta la struttura

    strcpy(game->owner, owner_name); // Registra il nome del creatore
    game->owner_sd = owner_sd; // Salva il socket del creatore

    // Inizializza mutex e variabile di condizione per la sincronizzazione della partita
    pthread_mutex_init(&(game->mutex_state), NULL);
    pthread_cond_init(&(game->cv_state), NULL);

    game->status = NEW_GAME; // Stato iniziale della partita
    game->join_request = false; // Nessuna richiesta ancora presente

    pthread_mutex_lock(&game_mutex); // Blocca l’accesso alla lista globale delle partite

    game->next_node = game_head; // Inserisce la nuova partita in cima alla lista

    // Se la precedente testa era anch’essa NEW_GAME, aggiorna il suo stato
    if (game_head != NULL && game_head->status == NEW_GAME)
    {
        game_head->status = WAITING; // Segnala che c'è una partita in attesa
    }

    game_head = game; // Imposta la nuova testa della lista

    pthread_mutex_unlock(&game_mutex); // Sblocca l’accesso alla lista
}

// ======================================================================
// Funzione: find_game_by_player
// Scopo: Cerca una partita a cui il socket specificato sta partecipando
// ======================================================================

struct GameNode* find_game_by_player(const int player_sd)
{
    struct GameNode *game = game_head; // Parte dalla testa della lista

    while (game != NULL)
    {
        if (game->owner_sd == player_sd || game->enemy_sd == player_sd)
        {
            return game; // Trovata partita associata al giocatore
        }
        game = game->next_node; // Avanza nella lista
    }

    return NULL; // Nessuna partita trovata per quel giocatore
}

// ============================================================================
// Funzione: find_game_by_index
// Scopo: Trova la partita corrispondente a un indice visualizzato nella lista
// ============================================================================

struct GameNode* find_game_by_index(const unsigned int index)
{
    unsigned int game_index = 0; // Contatore di partite compatibili
    pthread_mutex_lock(&game_mutex); // Protegge accesso alla lista

    struct GameNode *game = game_head;

    while (game != NULL)
    {
        if (game->status == WAITING || game->status == NEW_GAME)
        {
            game_index++; // Solo partite disponibili vengono contate
        }

        if (game_index == index) // Match con indice richiesto
        {
            pthread_mutex_unlock(&game_mutex);
            return game;
        }

        game = game->next_node; // Prossima partita
    }

    pthread_mutex_unlock(&game_mutex);
    return NULL; // Nessuna corrispondenza
}

// ===================================================================
// Funzione: delete_game
// Scopo: Rimuove una partita dalla lista globale e libera la memoria
// ===================================================================

void delete_game(struct GameNode *game)
{
    pthread_mutex_lock(&game_mutex); // Protezione concorrente

    if (game != NULL && game_head == game) // Se la partita da rimuovere è in testa
    {
        game_head = game_head->next_node; // Avanza la testa
        free(game); // Libera la memoria
        show_game_changement(); // Notifica la modifica (funzione esterna)
    }
    else if (game != NULL) // Se la partita è altrove nella lista
    {
        struct GameNode *tmp = game_head->next_node;

        while (tmp != NULL && tmp->next_node != game)
        {
            tmp = tmp->next_node; // Cerca il nodo precedente
        }

        if (tmp != NULL)
        {
            tmp->next_node = game->next_node; // Salta il nodo da rimuovere
            free(game); // Libera la memoria
            show_game_changement();
        }
    }

    pthread_mutex_unlock(&game_mutex); // Rilascia il lock
}
// ============================================================================
// Funzione: match_accepted
// Scopo: Invia una richiesta al creatore della partita e gestisce la risposta
// ============================================================================

bool match_accepted(struct GameNode *match, const int opponent, const char *name_opp)
{
    if (match->join_request == true)
    {
        return false; // Se c'è già una richiesta, rifiuta
    } 

    const int host = match->owner_sd; // Socket dell'host
    char buf[MAX_MESSAGE_SIZE];
    memset(buf, 0, MAX_MESSAGE_SIZE);
    strcat(buf, name_opp); // Aggiunge il nome dell'avversario
    strcat(buf, " wants to play with you. Do you accept? [y/n]\n"); // Messaggio di richiesta

    char response = '\0'; // Risposta dell’host

    if (send(opponent, "\nWaiting host...\n", 17, MSG_NOSIGNAL) < 0) // Notifica all'opponent
    {
        err_handler(opponent);
    }

    match->join_request = true; // Flag impostato: richiesta in corso

    if (send(host, buf, strlen(buf), MSG_NOSIGNAL) < 0) // Invia la richiesta all’host
    {
        err_handler(host);
        return false;
    }

    if (recv(host, &response, 1, 0) <= 0) // Riceve la risposta (y/n)
    {
        err_handler(opponent);
        return false;
    }

    if (response == 'Y' || response == 'y') // Se accetta
    {
        setup_running_match(match, host, name_opp, opponent); // Inizializza il match
        return true;
    }
    else // Se rifiuta
    {
        if (send(host, "Request refused, searching another opponent...\n", 51, MSG_NOSIGNAL) < 0)
        {
            err_handler(host);
        }
        match->join_request = false; // Reset flag
    }
    return false;
}

// ===================================================
// Funzione: setup_running_match
// Scopo: Inizializza i campi per avviare la partita
// ===================================================

void* setup_running_match(struct GameNode *match, const int host, const char *name_opp, const int opponent)
{
    strcpy(match->enemy, name_opp); // Salva il nome dell’avversario
    match->enemy_sd = opponent; // Salva il socket dell’avversario

    if (send(host, "Starting a game as host\n", 24, MSG_NOSIGNAL) < 0)
    {
        err_handler(host);
    }
    if (send(opponent, "Starting a game as opponent\n", 28, MSG_NOSIGNAL) < 0)
    {
        err_handler(opponent);
    }

    if (match != NULL)
    {
        match->status = RUNNING; // Stato aggiornato
        match->join_request = false; // Reset flag
        pthread_cond_signal(&(match->cv_state)); // Risveglia eventuali thread in attesa
    }
}

// ==============================================
// Funzione: play_game
// Scopo: Gestisce lo svolgimento di una partita
// ==============================================

void play_game(struct GameNode *gameData)
{
    const int host = gameData->owner_sd; // Socket del creatore della partita
    struct PlayerNode *owner = find_player_socket_desc(host); // Recupera struct del giocatore host
    owner->status = IN_GAME;         // Cambia lo stato del creatore in 'IN_GAME'
    owner->champion = true;          // L'host inizia sempre come campione

    // Informa l’host che si sta aspettando un avversario
    if (send(host, "\nWaiting opponent...\n", 31, MSG_NOSIGNAL) < 0)
    {
        err_handler(host); // In caso di errore termina la sessione dell’host
    }

    show_game_changement(); // Notifica visivamente agli altri client lo stato aggiornato

    pthread_mutex_lock(&(gameData->mutex_state)); // Blocca il mutex per sincronizzazione

    // Attende che il gioco venga segnato come RUNNING (cioè che l’avversario entri)
    while (gameData->status != RUNNING)
    {
        struct timespec waiting_time;
        clock_gettime(CLOCK_REALTIME, &waiting_time); // Tempo attuale
        waiting_time.tv_sec += 1; // Aspetta massimo 1 secondo prima di controllare di nuovo
        waiting_time.tv_nsec = 0;

        // Aspetta che la condizione `cv_state` venga segnalata (oppure scade dopo 1s)
        int all_results = pthread_cond_timedwait(&(gameData->cv_state), &(gameData->mutex_state), &waiting_time);

        if (all_results == ETIMEDOUT)
        {
            // Invia carattere nullo per notificare l’attesa ancora in corso
            if (send(host, "\0", 1, MSG_NOSIGNAL) < 0)
            {
                err_handler(host);
            }
                
        }
    }

    pthread_mutex_unlock(&(gameData->mutex_state)); // Rilascia il mutex dopo l’attesa

    const int opponent = gameData->enemy_sd; // Socket dell’avversario
    struct PlayerNode *opponent_struct = find_player_socket_desc(opponent); // Struct opponent
    opponent_struct->champion = false; // Inizialmente non è campione

    unsigned int round = 0; // Conta i turni

    do 
    {
        gameData->status = RUNNING; // Imposta stato partita
        round++;                    // Incrementa il numero del round
        show_game_changement();     // Aggiorna la visibilità delle partite

        // Variabili di supporto
        bool err = false;               // Flag per gestire errori di comunicazione
        char play = '\0';              // Mossa effettuata
        char host_result = NONE;       // Risultato del round per l'host
        char opponent_result = NONE;   // Risultato del round per l'avversario

        do 
        {
            if (round % 2 != 0) // Turno dispari → inizia l'host
            {
                // L'host effettua la sua mossa e comunica il risultato
                if (recv(host, &play, 1, 0) <= 0 || recv(host, &host_result, 1, 0) <= 0)
                {
                    err_handler(host);
                }

                // Invia la mossa all’avversario
                if (send(opponent, &NO_ERROR, 1, MSG_NOSIGNAL) < 0 || send(opponent, &play, 1, MSG_NOSIGNAL) < 0)
                {
                    err_handler(opponent); 
                    err = true; break;
                }

                if (host_result != NONE) break; // Il gioco può terminare se l’host ha già vinto o perso

                // L'avversario gioca
                if (recv(opponent, &play, 1, 0) <= 0 || recv(opponent, &opponent_result, 1, 0) <= 0)
                {
                    err_handler(opponent);
                    err = true; break;
                }

                // L’host riceve la risposta
                if (send(host, &NO_ERROR, 1, MSG_NOSIGNAL) < 0 || send(host, &play, 1, MSG_NOSIGNAL) < 0)
                {
                    err_handler(host);
                }
            }
            else // Turno pari → inizia l’avversario
            {
                // L'avversario effettua la mossa
                if (recv(opponent, &play, 1, 0) <= 0 || recv(opponent, &opponent_result, 1, 0) <= 0)
                {
                    err_handler(opponent);
                    err = true; break;
                }

                // Invia la mossa all'host
                if (send(host, &NO_ERROR, 1, MSG_NOSIGNAL) < 0 || send(host, &play, 1, MSG_NOSIGNAL) < 0)
                {
                    err_handler(host);
                }

                if (opponent_result != NONE) break;

                // L’host gioca
                if (recv(host, &play, 1, 0) <= 0 || recv(host, &host_result, 1, 0) <= 0)
                {
                    err_handler(host);
                }

                // L’avversario riceve la risposta
                if (send(opponent, &NO_ERROR, 1, MSG_NOSIGNAL) < 0 || send(opponent, &play, 1, MSG_NOSIGNAL) < 0)
                {
                    err_handler(opponent);
                    err = true; break;
                }
            }
        } while (host_result == NONE && opponent_result == NONE); // Continua finché nessuno ha vinto o perso

        if (err) // Se c’è stato un errore durante il round
        {
            owner->wins++;
            owner->champion = true;
            break;
        }

        // Caso in cui vince l'host
        if (host_result == WON || opponent_result == LOST)
        {
            owner->wins++;
            opponent_struct->losts++;
            owner->champion = true;
            opponent_struct->champion = false;
            opponent_struct->status = IN_LOBBY;
            pthread_cond_signal(&(opponent_struct->cv_state)); // Risveglia il thread in attesa
            break;
        }
        // Caso in cui vince l’avversario
        else if (host_result == LOST || opponent_result == WON)
        {
            owner->losts++;
            opponent_struct->wins++;
            owner->champion = false;
            opponent_struct->champion = true;
            opponent_struct->status = REQUESTING;
            pthread_cond_signal(&(opponent_struct->cv_state)); // Risveglia il thread in attesa
            break;
        }

        // Caso di pareggio
        owner->draws++;
        opponent_struct->draws++;
        gameData->status = END_GAME; // Segna la fine della partita
        show_game_changement(); // Notifica visiva

    } while (rematch(host, opponent)); // Loop per eventuale rivincita se entrambi accettano
}

// =================================================================
// Funzione: rematch
// Scopo: Gestisce la logica di richiesta rematch tra due giocatori
// =================================================================

bool rematch(const int host, const int opponent_sd)
{
    struct PlayerNode *opponent = find_player_socket_desc(opponent_sd); // Ottiene la struttura del giocatore avversario
    char opponent_response = '\0'; // Risposta dell'avversario
    char host_response = '\0'; // Risposta dell'host

    // Chiede all'avversario se vuole un'altra partita
    if (send(opponent_sd, "Rematch? [y/n]\n", 15, MSG_NOSIGNAL) < 0)
    {
        err_handler(opponent_sd);
    } 

    // Riceve la risposta dall'avversario
    if (recv(opponent_sd, &opponent_response, 1, 0) <= 0)
    {
        err_handler(opponent_sd);
    } 

    // Se l'avversario rifiuta, avvisa l'host
    if (opponent_response != 'Y' ) 
    {
        if (send(host, "Rematch refused by opponent\n", 28, MSG_NOSIGNAL) < 0)
        {
            err_handler(host);
        } 
    }
    else // Altrimenti chiede all'host se anche lui vuole il rematch
    {
        if (send(opponent_sd, "\nWaiting host...\n", 17, MSG_NOSIGNAL) < 0)
        {
            err_handler(opponent_sd);
        } 
        if (send(host, "Do you want a rematch? [y/n]\n", 29, MSG_NOSIGNAL) < 0)
        {
            err_handler(host);
        } 
        if (recv(host, &host_response, 1, 0) <= 0)
        {
            err_handler(host);
        } 
    }

    // Se l'host rifiuta
    if (host_response != 'Y')
    {
        if (host_response == 'N' || host_response == 'n')
        {
            if (send(opponent_sd, "Rematch refused by host\n", 23, MSG_NOSIGNAL) < 0)
            {
                err_handler(opponent_sd);
            } 
        }
        if (opponent != NULL)
        {
            opponent->status = REQUESTING; // Torna a richiedere un'altra partita
            pthread_cond_signal(&(opponent->cv_state)); // Risveglia l'avversario
        }
        return false; // Nessun rematch
    }
    else // Se entrambi accettano
    {
        if (send(opponent_sd, "Rematch accepted, ready for the next round\n", 43, MSG_NOSIGNAL) < 0)
        {
            err_handler(opponent_sd);
        } 
        if (send(host, "Rematch accepted, ready for the next round\n", 43, MSG_NOSIGNAL) < 0)
        {
            err_handler(host);
        } 
        return true; // Rematch confermato
    }
}

// =========================================================
// Funzione: send_game
// Scopo: Invia la lista aggiornata delle partite al client
// =========================================================

void send_game()
{
    struct PlayerNode *player = find_player_tid(pthread_self()); // Ottiene il giocatore corrente (in base a thread)
    const int client = player->player_sd; // Socket descriptor del client

    pthread_mutex_lock(&game_mutex); // Blocca l'accesso alla lista delle partite
    struct GameNode *momentary = game_head; // Inizio della lista

    char buff_out[MAX_MESSAGE_SIZE]; // Buffer messaggio
    memset(buff_out, 0, MAX_MESSAGE_SIZE);

    int i = 0; // Contatore degli ID
    char index_str[3];
    memset(index_str, 0, 3); // Pulisce stringa indice

    char game_state[28]; // Buffer per descrizione stato partita

    // Nessuna partita presente
    if (momentary == NULL) 
    {
        if (send(client, "\n\nThere aren't match actives now, write \"create\" for create ones or \"esc\" to exit\n", 85, MSG_NOSIGNAL) < 0)
        {
            err_handler(client);
        } 
    }
    else // Partite presenti
    {
        if (send(client, "\n~MATCH LIST~\n", 14, MSG_NOSIGNAL) < 0)
        {
            err_handler(client);
        } 
        while (momentary != NULL)
        {
            memset(buff_out, 0, MAX_MESSAGE_SIZE);
            memset(game_state, 0, 28);

            // Stato partita in formato leggibile
            switch (momentary->status)
            {
                case NEW_GAME:
                    strcpy(game_state, "just created\n");
                    break;
                case WAITING:
                    strcpy(game_state, "waiting for a player\n");
                    break;
                case RUNNING:
                    strcpy(game_state, "running\n");
                    break;
                case END_GAME:
                    strcpy(game_state, "ended\n");
                    break;
            }

            // Composizione messaggio da inviare
            strcat(buff_out, "\nHost: "); 
            strcat(buff_out, momentary->owner); 
            strcat(buff_out, "\nOpponent: ");
            strcat(buff_out, momentary->enemy); 
            strcat(buff_out, "\nGame Status: ");
            strcat(buff_out, game_state); 

            // Aggiunta dell'indice solo per partite disponibili
            if (momentary->status == NEW_GAME || momentary->status == WAITING)
            {
                i++;
                sprintf(index_str, "%u\n", i);
                strcat(buff_out, "Game Id: "); 
                strcat(buff_out, index_str);
            }

            // Invia la descrizione della partita
            if (send(client, buff_out, strlen(buff_out), MSG_NOSIGNAL) < 0)
            {
                err_handler(client);
            } 

            momentary = momentary->next_node; // Prossima partita
        }

        // Messaggio finale con istruzioni
        if (send(client, "\nJoin a waiting match by typing it's ID, write \"create\" to create ones or \"esc\" to exit\n", 92, MSG_NOSIGNAL) < 0)
        {
            err_handler(client);
        } 
    }
    pthread_mutex_unlock(&game_mutex); // Rilascia il lock
}

// ==============================================================================
// Funzione: quit
// Scopo: Chiede all'host se vuole accettare un altro giocatore dopo una partita
// ==============================================================================

bool quit(const int host)
{
    char response = '\0'; // Variabile per memorizzare la risposta dell'host

    // Invia il messaggio di richiesta rematch all'host
    if (send(host, "Do you want to accept another player? [y/n]\n", 44, MSG_NOSIGNAL) < 0)
    {
        err_handler(host); // Gestione errore se invio fallisce
    }

    // Riceve la risposta dall'host
    if (recv(host, &response, 1, 0) <= 0)
    {
        err_handler(host); // Gestione errore se ricezione fallisce
    } 

    // Se la risposta è 'Y' o 'y' => NON abbandona => ritorna false
    if (response == 'Y' || response == 'y')
    {
        return false;
    } 
    else // In caso contrario, esce dal ciclo
    {
        return true;
    } 
}

// =======================================================
// Funzione: player_create_head
// Scopo: Crea un nuovo nodo PlayerNode e lo inizializza
// =======================================================

struct PlayerNode* player_create_head(const char *player_name, const int client)
{
    struct PlayerNode *newHead = (struct PlayerNode *) malloc(sizeof(struct PlayerNode)); // Alloca nuova struttura giocatore

    check_error_player_head(newHead, client); // Verifica che l’allocazione sia andata a buon fine

    init_player_head(newHead, client, player_name); // Inizializza la struttura con nome e socket

    return newHead; // Ritorna il nodo pronto per essere inserito in lista
}

// ==============================================================================
// Funzione: find_player_socket_desc
// Scopo: Trova un giocatore attivo nella lista tramite il suo socket descriptor
// ==============================================================================

struct PlayerNode* find_player_socket_desc(const int soc_desc)
{
    struct PlayerNode *temporary = player_head; // Punta alla testa della lista giocatori

    // Scorre tutta la lista collegata
    while (temporary != NULL)
    {
        if (temporary->player_sd == soc_desc) // Se il socket corrisponde
        {
            return temporary; // Ritorna il giocatore
        }
        temporary = temporary->next_node; // Avanza al prossimo nodo
    } 

    return NULL; // Nessun giocatore trovato con quel socket
}

// =========================================================================
// Funzione: check_error_player_head
// Scopo: Verifica se l'allocazione del nodo giocatore è andata a buon fine
// =========================================================================

void* check_error_player_head(struct PlayerNode* newHead, const int client)
{
    if (newHead == NULL)
    {
        send(client, "error\n", 7, MSG_NOSIGNAL); // Notifica errore al client
        close(client); // Chiude il socket
        printf("Error during the creation of the player\n"); // Messaggio di log
        pthread_exit(NULL); // Termina il thread in modo sicuro
    }
}

// ============================================================
// Funzione: init_player_head
// Scopo: Inizializza un nodo PlayerNode con i valori del nuovo giocatore
// ============================================================

void* init_player_head(struct PlayerNode* newHead, const int client, const char* player_name)
{
    memset(newHead, 0, sizeof(struct PlayerNode)); // Azzeramento completo della struttura
    strcpy(newHead->name, player_name); // Copia il nome del giocatore nella struttura

    newHead->wins = 0; // Inizializza conteggi vittorie
    newHead->losts = 0; // Inizializza conteggi sconfitte
    newHead->draws = 0; // Inizializza conteggi pareggi

    pthread_mutex_init(&(newHead->mutex_state), NULL); // Inizializza mutex del giocatore
    pthread_cond_init(&(newHead->cv_state), NULL); // Inizializza condition variable del giocatore

    newHead->status = IN_LOBBY; // Stato iniziale nella lobby
    newHead->player_tid = pthread_self(); // Salva il thread ID del chiamante
    newHead->player_sd = client; // Associa socket client
    newHead->champion = false; // Nessuna vittoria corrente

    pthread_mutex_lock(&player_mutex); // Protegge la lista globale
    newHead->next_node = player_head; // Inserisce in testa alla lista
    player_head = newHead;
    pthread_mutex_unlock(&player_mutex); // Rilascia la lock
}

// ===================================================
// Funzione: find_player_tid
// Scopo: Trova un giocatore tramite il suo thread ID
// ===================================================

struct PlayerNode* find_player_tid(const pthread_t tid)
{
    struct PlayerNode *temporary = player_head; // Inizia dalla testa

    while (temporary != NULL)
    {
        if (temporary->player_tid == tid) // Confronta thread ID
        {
            return temporary; // Giocatore trovato
        }
        temporary = temporary->next_node; // Passa al nodo successivo
    }

    return NULL; // Nessun giocatore con quel thread ID
}

// ===================================================================
// Funzione: delete_player
// Scopo: Rimuove un giocatore dalla lista collegata e libera memoria
// ===================================================================

// Funzione per rimuovere un giocatore dalla lista
void delete_player(struct PlayerNode *node)
{
    // Blocco il mutex per evitare accessi concorrenti alla lista
    pthread_mutex_lock(&player_mutex);
    // Caso 1: il nodo da eliminare è la testa della lista
    if (node != NULL && player_head == node)
    {
         // Sposto la testa al nodo successivo
        player_head = player_head->next_node;
        // Libero la memoria allocata per il nodo
        free(node);
    }
    else
    { 
        // Caso 2: il nodo da eliminare è in mezzo o in fondo alla lista
        if (node != NULL){ delete_node_beetween(player_head, node);}
        // Sblocco il mutex dopo la modifica alla lista
        pthread_mutex_unlock(&player_mutex);
    }
}

void* delete_node_beetween(struct PlayerNode* player_head, struct PlayerNode* node)
{
    struct PlayerNode *temporaneo = player_head;
    // Cerco il nodo precedente a quello da eliminare
    while (temporaneo->next_node != node && temporaneo != NULL)
    {
        temporaneo = temporaneo->next_node;
    }
    // Lo scollego dalla lista saltando il nodo da eliminare
    temporaneo->next_node = node->next_node;
    // Libero la memoria
    free(node);
}

// =======================================================================================================
// Funzione: show_game_changement
// Scopo: Notifica a tutti i giocatori in lobby (eccetto il chiamante) un cambiamento nella lista partite
// =======================================================================================================

void show_game_changement()
{ 
    const pthread_t sender_tid = pthread_self(); // Thread ID del chiamante
    pthread_t recv_tid = 0;

    pthread_mutex_lock(&player_mutex); // Protegge accesso alla lista
    struct PlayerNode *momentary = player_head;
    pthread_mutex_unlock(&player_mutex);

    while (momentary != NULL)
    {
        recv_tid = momentary->player_tid;

        // Invia segnale solo ad altri giocatori in lobby
        if (recv_tid != sender_tid && momentary->status == IN_LOBBY)
        {
            pthread_kill(recv_tid, SIGUSR1); // Invia segnale SIGUSR1 al thread del giocatore
        } 
        momentary = momentary->next_node; // Passa al giocatore successivo
    }
}

// =========================================================================
// Funzione: err_handler
// Scopo: Gestisce errori di comunicazione o disconnessione di un giocatore
// =========================================================================

void err_handler(const int player)
{
    pthread_t thread_id = 0;
    struct PlayerNode *current_player = find_player_socket_desc(player); // Trova il giocatore corrispondente

    if (current_player != NULL) 
    {
        thread_id = current_player->player_tid; // Salva il suo thread ID
    }

    struct GameNode *match = find_game_by_player(player); // Trova eventuale partita associata

    if (match != NULL) 
    {
        handle_player_disconnection(match, player); // Gestisce disconnessione all'interno della partita
    }

    if (current_player != NULL)
    {
        pthread_kill(thread_id, SIGALRM); // Invia segnale di terminazione al thread del giocatore
    } 
}

// ========================================================================================
// Funzione: handle_player_disconnection
// Scopo: Gestisce la disconnessione di un giocatore durante una partita o prima che inizi
// ========================================================================================

void* handle_player_disconnection(struct GameNode* match, const int player)
{
    // Se la partita è in corso
    if (match->status == RUNNING)
    {
        // Se il giocatore disconnesso è l'host
        if (player == match->owner_sd) 
        {
            // Invia un errore all'avversario
            send(match->enemy_sd, &ERROR, 1, MSG_NOSIGNAL);
            
            // Recupera la struttura del giocatore avversario
            struct PlayerNode *opponent = find_player_socket_desc(match->enemy_sd);
            
            // Se l'avversario è valido
            if (opponent != NULL)
            {
                opponent->wins++;                      // Incrementa le vittorie
                opponent->champion = true;             // Lo imposta come vincitore
                opponent->status = REQUESTING;         // Stato aggiornato a "richiesta in corso"
                pthread_cond_signal(&(opponent->cv_state)); // Risveglia il thread in attesa
            }
            delete_game(match); // Elimina la partita in corso
        }
        else
        {
            // Se a disconnettersi è l'opponente, invia errore all'host
            send(match->owner_sd, &ERROR, 1, MSG_NOSIGNAL);
        }
    }
    else
    {
        // Se la partita non è in corso, semplicemente la elimina
        delete_game(match); 
    }
}

// ========================================================================
// Funzione: handle_player_disconnection
// Scopo: Gestisce il segnale SIGALRM: chiude la connessione del giocatore
// ========================================================================
 
void sigalrm_h()
{
    struct PlayerNode *player = find_player_tid(pthread_self()); // Recupera il giocatore corrente
    close(player -> player_sd);        // Chiude la socket del client
    delete_player(player);             // Rimuove il giocatore dalla lista
    pthread_exit(NULL);                // Termina il thread
}

// ================================================================================
// Funzione: handle_player_disconnection
// Scopo: Gestisce il segnale SIGTERM per Docker: termina il processo con successo
// ================================================================================

void docker_SIGTERM_h()
{
    exit(EXIT_SUCCESS);
}

// =================================================================================
// Funzione: handle_player_disconnection
// Scopo: Informa gli altri giocatori della presenza di un nuovo utente nella lobby
// =================================================================================

void show_new_player()
{
    pthread_mutex_lock(&player_mutex);            // Lock per accesso alla lista
    struct PlayerNode *momentary = player_head;   // Inizio della lista giocatori
    pthread_mutex_unlock(&player_mutex);          // Unlock immediato

    const pthread_t sender_tid = pthread_self();  // Thread del mittente (nuovo giocatore)
    pthread_t recv_tid = 0;

    while (momentary != NULL)
    {
        recv_tid = momentary -> player_tid;       // Ottiene il thread del giocatore attuale
        // Se il thread è diverso da chi ha appena fatto accesso e il giocatore è in lobby
        if (recv_tid != sender_tid && (momentary -> status == IN_LOBBY))
        {
            pthread_kill(recv_tid, SIGUSR2);      // Invia segnale SIGUSR2 al thread
        } 
        momentary = momentary -> next_node;       // Passa al prossimo giocatore
    }
}

// =================================================================================
// Funzione: handle_player_disconnection
// Scopo: Mostra un messaggio ai client quando un nuovo giocatore entra nella lobby
// =================================================================================

void handler_new_player()
{
    pthread_mutex_lock(&player_mutex);            // Blocca la lista dei giocatori
    struct PlayerNode *momentary = player_head;   // Inizia a scorrere i giocatori
    pthread_mutex_unlock(&player_mutex);          // Sblocca subito dopo

    char mess[MAX_MESSAGE_SIZE];                  // Buffer per il messaggio
    memset(mess, 0, MAX_MESSAGE_SIZE);            // Pulizia iniziale del buffer

    if (momentary != NULL)
    {
        strcat(mess, momentary -> name);          // Aggiunge il nome del giocatore
        strcat(mess, " entered in lobby!\n");    // Aggiunge il testo fisso

        struct PlayerNode *player = find_player_tid(pthread_self()); // Ottiene chi riceve

        // Invia il messaggio al socket del giocatore attuale
        if (send(player -> player_sd, mess, strlen(mess), MSG_NOSIGNAL) < 0)
        {
            err_handler(player -> player_sd);     // Gestisce eventuali errori
        } 
    }
}