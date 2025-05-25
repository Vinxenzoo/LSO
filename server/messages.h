#ifndef MESSAGES_H
#define MESSAGES_H


//Server
#define SERVER_ERROR "01\n"

#define VICTORY_M "\n1A\n"
#define LOST_M    "\n1B\n"
#define DRAW_M    "\n1C\n"


#define INSERT_PLAYER_NAME "20\n"
#define INVALID_PLAYER_NAME "21\n"
#define ALREADY_TAKEN_NAME "22\n"

#define REGISTRATION_COMPLETED "23\n"

#define CREATE_GAME_FAILED "30\n"
#define GAME_NOT_FOUND "33\n"
#define GAME_FOUND "34\n"
#define RETURN_LOBBY "35\n"
#define JOIN_REQUEST_REJECTED "37\n"
#define ANOTHER_PLAYER_REQUEST "38\n"
#define WAITING_OWNER_RESPONSE "39\n"

#define STARTING_GAME_OWNER "3A\n"
#define STARTING_GAME_ENEMY "3B\n"
#define REQUEST_DENIED "3C\n"


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
