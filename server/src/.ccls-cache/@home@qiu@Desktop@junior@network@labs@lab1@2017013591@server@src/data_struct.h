#ifndef _DATA_STRUCT_H
#define _DATA_STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define true    1
#define false   0
#define bool    int
#define MAX_LEN 8192

#define ARG_PORT 1
#define ARG_ROOT 2

#define CMD_EMPTY_ERROR      -1
#define ARG_SYNTAX_ERROR     -2
#define CMD_UNKNOWN_ERROR    -3
#define LOGIN_REQUIRED_ERROR -4

extern const char* cmd_const[];


typedef enum {
    USER, PASS, RETR, STOR, REST,
    ABOR, QUIT, SYST, TYPE, PORT,
    PASV, MKD,  CWD,  PWD,  CDUP,
    LIST, RMD,  RNFR, RNTO, DELE,
    UNKNOWN,
} COMMAND;


typedef struct {
    bool  loggedin;
    char* username;
    char* password;
} Client;


typedef struct {
    COMMAND cmd;
    char* arg;
} Command;


typedef struct Session {
    // unique id in session list
    int id;
    // control connection socket
    int ctrlSock;
    // data connection socket
    int dataSock;
    // file start position set by command REST
    int pos;
    // number of successfully downloaded files
    int down_files;
    // number of successfully uploaded files
    int up_files;
    // total size of all successfully downloaded files
    long long down_size;
    // total size of all successfully uploaded files
    long long up_size;
    // set to 1 after receiving ABOR or QUIT
    int quitFlag;
    bool modePort;
    bool modePasv;
    Client client;
    Command command;
    Command precmd;
    // where the data connection will connect to
    struct sockaddr_in dataAddr;
    // next session
    struct Session* next;
} Session, *SNode;


// all client sessions in progress
typedef struct {
    // the size of the session list
    int size;
    // the first session node
    SNode head;
    // the last session node
    SNode tail;
} SList;

extern bool assignCmd(Command* dst, Command* src);

extern void strrpl(char *str, char srcch, char dstch);

extern bool init(SList* slist);
extern bool clear(SList* slist);
extern bool del(SList* slist, int id);
extern bool add(SList* slist, SNode snode);
extern bool assign(SNode dst, SNode src);
extern bool pop(SList* slist);
extern bool closeSession(Session* session);
extern void show(Session *session);
extern SNode get(SList* slist, int id);
extern SNode find(SList* slist, int sockfd);

#endif
