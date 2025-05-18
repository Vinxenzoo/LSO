#ifndef MESSAGES_H
#define MESSAGES_H


//Server
#define SERVER_ERROR "01\n"

#define VICTORY "1A\n"
#define LOST    "1B\n"
#define DRAW    "1C\n"


#define INSERT_PLAYER_NAME "20\n"
#define INVALID_PLAYER_NAME "21\n"
#define ALREADY_TAKEN_NAME "22\n"

#define REGISTRATION_COMPLETED "23\n"

#define CREATE_GAME_FAILED "30\n"
#define GAME_FOUND "34\n"
#define RETURN_LOBBY "35\n"
#define JOIN_REQUEST_REJECTED "37\n"
#define ANOTHER_PLAYER_REQUEST "38\n"

//Client
#define EXIT "51\n"
#define CREATE "52\n"



/*
--------------Messages Examples---------------
Server/Stats
s[wins]-[lost]-[draws]\n
ex: s100-10-2

Server/Join Request
r[name]
rDido

*/


#endif
