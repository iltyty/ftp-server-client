#ifndef _HANDLE_CLIENT_H
#define _HANDLE_CLIENT_H

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "parse_cmd.h"
#include "itrpt_handle.h"
#include "data_struct.h"

/* print log */
extern void print_log(char *func, char *cmd);

/* Basic communication processes between server and clients */
extern int getClientSocket(int listenfd);
extern int handleCommand(Session* session);
extern int dealWithClient(Session* session);

extern int readCommand(int contfd, char* recBuf);
extern int handleErrorCmd(int code, Session* session);
extern int sendResp(int sockfd, char *resBuf);

extern int getIpAddress(char *host);

extern void resetTransMode(Session* session);
/* Command PORT */
extern int handleCmdPort(Session* session);
extern int parsePortArgs(char* cmd, int res[6]);
extern int initDataSockInfo(char *host, int port, Session* session);

/* Command PASV */
extern int retRandomPort();
extern int portAvailable(int port);
extern int handleCmdPasv(Session* session);

extern int handleCmdMkd(Session* session);
extern int handleCmdCwd(Session* session);
extern int handleCmdPwd(Session* session);
extern int handleCmdRmd(Session* session);
extern int handleCmdUser(Session* session);
extern int handleCmdPass(Session* session);
extern int handleCmdRetr(Session* session);
extern int handleCmdStor(Session* session);
extern int handleCmdRest(Session* session);
extern int handleCmdSyst(Session* session);
extern int handleCmdType(Session* session);
extern int handleCmdQuit(Session* session);
extern int handleCmdAbor(Session* session);
extern int handleCmdCdup(Session* session);
extern int handleCmdList(Session* session);
extern int handleCmdRnfr(Session* session);
extern int handleCmdRnto(Session* session);
extern int handleCmdDele(Session* session);
extern int handleCmdUnkn(Session* session);

#endif
