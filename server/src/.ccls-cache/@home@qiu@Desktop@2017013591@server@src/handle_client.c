#include "handle_client.h"

extern SList sessions;

const char* cmd_const[] = {
    "USER", "PASS", "RETR", "STOR", "REST",
    "ABOR", "QUIT", "SYST", "TYPE", "PORT",
    "PASV", "MKD",  "CWD",  "PWD",  "CDUP",
    "LIST", "RMD",  "RNFR", "RNTO", "DELE",
    "UNKNOWN",
};

void print_log(char *func, char *cmd) {
    // time_t cur_time;
    // struct tm *tmp_ptr = NULL;
    // char log_msg[100];
    //
    // time(&cur_time);
    // tmp_ptr = localtime(&cur_time);
    // sprintf(log_msg, "%d-%02d-%02d %02d:%02d:%02d [-] %s", (1900 + tmp_ptr->tm_year), (1 + tmp_ptr->tm_mon),
    //         tmp_ptr->tm_mday, tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec, func);
    //
    // printf("\033[31m%s\033[0m: \033[32m%s\033[0m\n", log_msg, cmd);
}


// read the data from client
int readCommand(int connfd, char* recBuf) {
    memset(recBuf, 0, MAX_LEN);

    return recv(connfd, recBuf, MAX_LEN - 1, 0);
}


// send msg back to client
int sendResp(int sockfd, char *resBuf) {
    int res = send(sockfd, resBuf, strlen(resBuf), 0);
    if (res <= 0) {
        print_log("Received error", "Error send response to client.");
        del(&sessions, find(&sessions, sockfd)->id);
        exit(-1);
    }
    return res;
}


// get the current ip address
int getIpAddress(char *res) {
    char host[200];
    gethostname(host, 200);
    struct hostent *hp = gethostbyname(host);
    inet_ntop(AF_INET, (struct in_addr*)hp->h_addr_list[0], res, 100);
    return 1;
}


// return the client socket
int getClientSocket(int listenfd) {
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(struct sockaddr_in);
    int connfd = accept(listenfd, (struct sockaddr*)&clientAddr, &len);

    if (connfd == -1) {
        char err[200];
        sprintf(err, "Error accept(): %s(%d)\n", strerror(errno), errno);
        print_log("Receiver error", err);
        return -1;
    }

    char tmp[100];
    char host[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), host, INET_ADDRSTRLEN);
    sprintf(tmp, "%s:%d", host, clientAddr.sin_port);
    print_log("Connected to", tmp);

    return connfd;
}


// handle error command
int handleErrorCmd(int code, Session* session) {
    char *resBuf;
    switch (code) {
        case CMD_EMPTY_ERROR:
        case CMD_UNKNOWN_ERROR:
            resBuf = "500 Syntax error, command unrecognized.\r\n";
            sendResp(session->ctrlSock, resBuf);
            return 1;
        case ARG_SYNTAX_ERROR:
            resBuf = "501 Syntax error in parameters!\r\n";
            sendResp(session->ctrlSock, resBuf);
            return 1;
        case LOGIN_REQUIRED_ERROR:
            resBuf = "530 Please login first!\r\n";
            sendResp(session->ctrlSock, resBuf);
            return 1;
        default:
            return 0;
    }
}


// deal with one client
int dealWithClient(Session* session) {
    int res;
    char recBuf[MAX_LEN];

    char* resBuf = "220 Anonymous FTP server ready.\r\n";
    sendResp(session->ctrlSock, resBuf);

    while (!session->quitFlag) {
        int p = readCommand(session->ctrlSock, recBuf);
        if (!p) {
            // del(&sessions, session->id);
            print_log("Received error", "read 0");
            close(session->ctrlSock);
            close(session->dataSock);
            return 0;
        }

        res = parseCommand(recBuf, session);
        char cmd[100];
        sprintf(cmd, "%s", cmd_const[session->command.cmd]);
        print_log("Received command", cmd);
        if (session->command.arg) {
            print_log(cmd, session->command.arg);
        } else {
            print_log(cmd, "NULL");
        }
        if (res < 0) {
            handleErrorCmd(res, session);
        } else {
            res = handleCommand(session);
            if (res < 0) {
                print_log("Received error", "Error handling client command.");
                // del(&sessions, session->id);
                close(session->ctrlSock);
                close(session->dataSock);
                return 0;
            }
            assignCmd(&(session->precmd), &(session->command));
        }

    }
    close(session->ctrlSock);
    close(session->dataSock);
    return 0;
}


// handle the client request
int handleCommand(Session* session) {
    switch (session->command.cmd) {
        case PWD:
            return handleCmdPwd(session);
        case CWD:
            return handleCmdCwd(session);
        case MKD:
            return handleCmdMkd(session);
        case RMD:
            return handleCmdRmd(session);
        case USER:
            return handleCmdUser(session);
        case PASS:
            return handleCmdPass(session);
        case RETR:
            return handleCmdRetr(session);
        case STOR:
            return handleCmdStor(session);
        case REST:
            return handleCmdRest(session);
        case ABOR:
        case QUIT:
            return handleCmdQuit(session);
        case SYST:
            return handleCmdSyst(session);
        case TYPE:
            return handleCmdType(session);
        case PORT:
            return handleCmdPort(session);
        case PASV:
            return handleCmdPasv(session);
        case CDUP:
            return handleCmdCdup(session);
        case LIST:
            return handleCmdList(session);
        case RNFR:
            return handleCmdRnfr(session);
        case RNTO:
            return handleCmdRnto(session);
        case DELE:
            return handleCmdDele(session);
        default:
            return handleCmdUnkn(session);
    }
}


/****************/
/* command USER */
/****************/

int handleCmdUser(Session* session) {
    char resBuf[100];

    if (session->client.loggedin) {
        sprintf(resBuf, "201 You are already logged in!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (session->client.username) {
        sprintf(resBuf, "530 Please send your complete e-mail address as password!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (!strcmp(session->command.arg, "anonymous")) {
        session->client.username = "anonymous";
        sprintf(resBuf, "331 Guest login ok, send your complete e-mail address as password.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 1;
    } else {
        sprintf(resBuf, "530 Username \"%s\" not acceptable.\r\n", session->command.arg);
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }
}


/****************/
/* command PASS */
/****************/

int handleCmdPass(Session* session) {
    char resBuf[100];

    if (session->client.loggedin) {
        sprintf(resBuf, "201 You are already logged in!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (!session->client.username) {
        sprintf(resBuf, "530 Please input username first!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    session->client.password = (char*) malloc(sizeof(char) * strlen(session->command.arg));
    session->client.loggedin = true;
    strcpy(session->client.password, session->command.arg);

    sprintf(resBuf, "230 User %s logged in, proceed:\r\n", session->client.username);
    sendResp(session->ctrlSock, resBuf);

    return 1;
}


void resetTransMode(Session* session) {
    close(session->dataSock);
    session->dataSock = 0;
    session->modePort = false;
    session->modePasv = false;
}


/****************/
/* command PORT */
/****************/

int handleCmdPort(Session* session) {
    char resBuf[100];
    int args[6];

    resetTransMode(session);

    int res = parsePortArgs(session->command.arg, args);
    if (!res) {
        sprintf(resBuf, "501 Syntax error in parameters!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    int port = args[4] * 256 + args[5];
    if (port >= 65535 || port <= 0) {
        sprintf(resBuf, "500 Invalid PORT command!\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }
    char host[100];
    sprintf(host, "%d.%d.%d.%d", args[0], args[1], args[2], args[3]);
    initDataSockInfo(host, port, session);

    sprintf(resBuf, "200 Entering Port Mode (%d,%d,%d,%d,%d,%d)\r\n", args[0], args[1], args[2], args[3], args[4], args[5]);

    sendResp(session->ctrlSock, resBuf);
    session->modePort = true;

    return 1;
}


// parse the six args in PORT command
int parsePortArgs(char* cmd, int args[6]) {
    if (!cmd) {
        return false;
    }
    int cnt = 0;
    char* tmp = (char*) malloc(sizeof(char) * strlen(cmd));
    strcpy(tmp, cmd);

    for (char* arg = strsep(&tmp, ","); arg != NULL; arg = strsep(&tmp, ",")) {
        args[cnt++] = atoi(arg);
    }

    free(tmp);

    if (cnt != 6) {
        return false;
    }
    return true;
}


// initialize client address with args parsed before
int initDataSockInfo(char *host, int port, Session* session) {
    int datafd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (datafd == -1) {
        char err[200];
        sprintf(err, "Error data socket(): %s(%d).", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }
    session->dataSock = datafd;

    memset(&(session->dataAddr), 0, sizeof(session->dataAddr));
    session->dataAddr.sin_family = AF_INET;
    session->dataAddr.sin_port = htons(port);
    session->dataAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int res = inet_pton(AF_INET, host, &(session->dataAddr.sin_addr));
    if (res <= 0) {
        char err[200];
        sprintf(err, "Error data socket inet_pton(): %s(%d).", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }

    return 1;
}


/****************/
/* command PASV */
/****************/

int handleCmdPasv(Session* session) {
    char resBuf[200];
    int port = retRandomPort();
    int p1 = port / 256;
    int p2 = port % 256;
    char tmp[10];
    sprintf(tmp, "%d", port);
    print_log("port: ", tmp);

    resetTransMode(session);
    // session->modePasv = false;
    // session->modePasv = false;

    char host[100];
    getIpAddress(host);
    initDataSockInfo(host, port, session);

    int n = 1;
    if ((n = setsockopt(session->dataSock, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int))) < 0) {
        print_log("Received error", "Error set socket option.");
    }

    // bind to listening socket
    if (bind(session->dataSock, (struct sockaddr*)&(session->dataAddr), sizeof(session->dataAddr)) == -1) {
        char err[200];
        sprintf(err, "Error data socket bind(): %s(%d).", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }

    // start the listening socket
    if (listen(session->dataSock, 10) == -1) {
        char err[200];
        sprintf(err, "Error data socket listen(): %s(%d).", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }

    strrpl(host, '.', ',');
    sprintf(resBuf, "227 Entering Passive Mode (%s,%d,%d)\r\n", host, p1, p2);
    sendResp(session->ctrlSock, resBuf);

    session->modePasv = true;

    return 1;
}

// return a random port between 20000 and 65535
int retRandomPort() {
    int port = rand() % 45536 + 20000;
    // if (portAvailable(port)) {
    //     return port;
    // }

    // continue generating until find an available port
    // return retRandomPort();
    return port;
}

// check whether the specified port is available
int portAvailable(int port) {
    struct sockaddr_in addr;
    int testfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // 127.0.0.1
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(testfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        // the port is not in use
        return true;
    }
    return false;
}


/****************/
/* command RETR */
/****************/

int handleCmdRetr(Session* session) {
    int res;
    char resBuf[200];
    char* filename;
    struct stat statbuf;

    int datafd;
    socklen_t len = sizeof(struct sockaddr_in);
    if (session->modePasv || session->modePort) {
        filename = session->command.arg;
        res = stat(filename, &statbuf);
        if (res == 0) {
            sprintf(resBuf, "150 Opening BINARY mode data connection for \"%s\" (%ld bytes).\r\n", filename, statbuf.st_size);
        } else {
            sprintf(resBuf, "150 Opening BINARY mode data connection for \"%s\".\r\n", filename);
        }
        sendResp(session->ctrlSock, resBuf);
    }

    if (session->modePasv) {
        datafd = accept(session->dataSock, (struct sockaddr*)&(session->dataAddr), &len);
        if (datafd < 0) {
            sprintf(resBuf, "425 Can't open data connection.\r\n");
            sendResp(session->ctrlSock, resBuf);
            return 0;
        }
    } else if (session->modePort) {
        res = connect(session->dataSock, (struct sockaddr*)&(session->dataAddr), sizeof(session->dataAddr));
        if (res < 0) {
            sprintf(resBuf, "425 Can't open data connection.\r\n");
            sendResp(session->ctrlSock, resBuf);
            return 0;
        }
    } else {
        sprintf(resBuf, "425 Please specify data transfer mode(PORT or PASV) first.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (res < 0 || datafd < 0) {
        sprintf(resBuf, "425 Can't open data connection.\r\n");
        res = sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    // FILE *file = fopen(filename, "rb");
    int file = open(filename, O_RDONLY, S_IRUSR);
    if (!file) {
        sprintf(resBuf, "451 Error opening file %s\r\n", filename);
        res = sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (session->pos) {
        /* fseek(file, session->pos, SEEK_SET); */
        lseek(file, session->pos, SEEK_SET);
    } else {
        lseek(file, 0, SEEK_SET);
    }

    int remain = statbuf.st_size;
    if (session->modePort) {
        int sent;
        while ((sent = sendfile(session->dataSock, file, NULL, MAX_LEN) > 0) && remain > 0) {
            remain -= sent;
        }
    } else if (session->modePasv) {
        int sent;
        while ((sent = sendfile(datafd, file, NULL, MAX_LEN) > 0) && remain > 0) {
            remain -= sent;
        }
        close(datafd);
    }
    close(file);

    resetTransMode(session);

    session->pos = 0;
    session->down_files++;
    session->down_size += statbuf.st_size;

    sprintf(resBuf, "226 File \"%s\" transfer completed!\r\n", filename);
    res = sendResp(session->ctrlSock, resBuf);

    return 1;
}


/****************/
/* command STOR */
/****************/

int handleCmdStor(Session* session) {
    int res;
    char resBuf[200];
    char *filename;
    struct stat statbuf;

    int datafd;
    socklen_t len = sizeof(struct sockaddr_in);
    if (session->modePasv || session->modePort) {
        filename = session->command.arg;
        res = stat(filename, &statbuf);
        if (res == 0) {
            sprintf(resBuf, "150 Opening BINARY mode data connection for \"%s\" (%ld bytes).\r\n", filename, statbuf.st_size);
        } else {
            sprintf(resBuf, "150 Opening BINARY mode data connection for \"%s\".\r\n", filename);
        }
        sendResp(session->ctrlSock, resBuf);
    }

    if (session->modePasv) {
        datafd = accept(session->dataSock, (struct sockaddr*)&(session->dataAddr), &len);
        if (datafd < 0) {
            sprintf(resBuf, "425 Can't open data connection.\r\n");
            sendResp(session->ctrlSock, resBuf);
            return 0;
        }
    } else if (session->modePort) {
        res = connect(session->dataSock, (struct sockaddr*)&(session->dataAddr), sizeof(session->dataAddr));
        if (res < 0) {
            sprintf(resBuf, "425 Can't open data connection.\r\n");
            sendResp(session->ctrlSock, resBuf);
            return 0;
        }
    } else {
        sprintf(resBuf, "425 Please specify data transfer mode(PORT or PASV) first.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }


    FILE *file = fopen(filename, "wb");

    if (!file) {
        sprintf(resBuf, "451 Error opening file %s\r\n", filename);
        res = sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    long long recvSize = 0;
    char tmp[MAX_LEN + 1];
    if (session->modePasv) {
        while (true) {
            res = readCommand(datafd, tmp);
            if (res > 0) {
                recvSize += res;
                fwrite(tmp, sizeof(char), res, file);
            } else {
                close(datafd);
                break;
            }
        }
    } else if (session->modePort) {
        while (true) {
            res = readCommand(datafd, tmp);
            if (res > 0) {
                recvSize += res;
                fwrite(tmp, sizeof(char), res, file);
            } else {
                break;
            }
        }
    }

    session->up_files++;
    session->up_size += recvSize;
    resetTransMode(session);

    fclose(file);

    sprintf(resBuf, "226 File \"%s\" store succeeds!\r\n", filename);
    res = sendResp(session->ctrlSock, resBuf);

    return 1;
}


/****************/
/* command REST */
/****************/

int handleCmdRest(Session* session) {
    char resBuf[100];
    session->pos = atoi(session->command.arg);
    sprintf(resBuf, "350 File start position set to %d\r\n", session->pos);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command SYST */
/****************/

int handleCmdSyst(Session* session) {
    char* resBuf = "215 UNIX Type: L8\r\n";
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command TYPE */
/****************/

int handleCmdType(Session* session) {
    char* resBuf;

    if (strcmp(session->command.arg, "I")) {
        resBuf = "504 Parameter format not supported!\r\n";
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    resBuf = "200 Type set to I.\r\n";
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/***************/
/* command PWD */
/***************/

int handleCmdPwd(Session* session) {
    char *resBuf, *path = NULL;

    path = getcwd(path, 0);
    resBuf = (char*) malloc(sizeof(char) * (strlen(path) + 10));
    sprintf(resBuf, "257 \"%s\"\r\n", path);

    sendResp(session->ctrlSock, resBuf);
    if (path) {
        free(path);
    }
    if (resBuf) {
        free(resBuf);
    }

    return 1;
}


/***************/
/* command CWD */
/***************/

int handleCmdCwd(Session* session) {
    char resBuf[200];

    int res = chdir(session->command.arg);
    if (res < 0) {
        sprintf(resBuf, "550 No such file or directory.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "250 Working directory changed to \"%s\"\r\n", session->command.arg);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command CDUP */
/****************/

int handleCmdCdup(Session* session) {
    char resBuf[200];
    char *path = NULL;

    int res = chdir("..");
    if (res < 0) {
        sprintf(resBuf, "550 No such file or directory.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    path = getcwd(path, 0);

    sprintf(resBuf, "250 Working directory changed to %s\r\n", path);
    sendResp(session->ctrlSock, resBuf);

    if (path) {
        free(path);
    }
    return 1;
}


/***************/
/* command MKD */
/***************/

int handleCmdMkd(Session* session) {
    char resBuf[100];

    int res = mkdir(session->command.arg, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (res < 0) {
        sprintf(resBuf, "550 Mkdir failed.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "250 \"%s\"\r\n", session->command.arg);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/***************/
/* command RMD */
/***************/

int handleCmdRmd(Session* session) {
    char resBuf[100];

    int res = rmdir(session->command.arg);
    if (res < 0) {
        sprintf(resBuf, "550 Rmdir failed.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "250 directory \"%s\" removed.\r\n", session->command.arg);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command LIST */
/****************/

int handleCmdList(Session* session) {
    char resBuf[100];
    unsigned len = sizeof(struct sockaddr_in);

    int res = 0;
    int datafd = 0;

    if (session->modePasv || session->modePort) {
        sprintf(resBuf, "150 Opening BINARY mode data connection.\r\n");
        sendResp(session->ctrlSock, resBuf);
    }

    if (session->modePasv) {
        datafd = accept(session->dataSock, (struct sockaddr*)&(session->dataAddr), &len);
    } else if (session->modePort) {
        res = connect(session->dataSock, (struct sockaddr*)&(session->dataAddr), sizeof(session->dataAddr));
    } else {
        sprintf(resBuf, "425 Please specify data transfer mode(PORT or PASV) first.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    if (res < 0 || datafd < 0) {
        sprintf(resBuf, "425 Can't open data connection.\r\n");
        res = sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    FILE *pipe = popen("/bin/ls -al", "r");
    if (!pipe) {
        print_log("Received error", "Error opening pipe for output.");
        return -1;
    }

    char tmp[1024];
    while (fgets(tmp, sizeof(tmp) - 1, pipe) != NULL) {
        if (session->modePasv) {
            sendResp(datafd, tmp);
        } else if(session->modePort) {
            sendResp(session->dataSock, tmp);
        }
    }

    pclose(pipe);
    close(datafd);
    resetTransMode(session);

    sprintf(resBuf, "226 Transfer finished.\r\n");
    sendResp(session->ctrlSock, resBuf);

    return 1;
}


/****************/
/* command RNFR */
/****************/

int handleCmdRnfr(Session* session) {
    char resBuf[100];
    int res = access(session->command.arg, 0);
    if (res == -1) {
        // the specified file doesn't exist
        sprintf(resBuf, "450 File \"%s\" does not exist.\r\n", session->command.arg);
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "350 Preparing to rename file \"%s\".\r\n", session->command.arg);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command RNTO */
/****************/

int handleCmdRnto(Session* session) {
    if (!session->precmd.cmd || session->precmd.cmd != RNFR) {
        char *resBuf = "503 Please input command RNFR first.\r\n";
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    char resBuf[100];

    int res = rename(session->precmd.arg, session->command.arg);
    if (res == -1) {
        sprintf(resBuf, "550 Rename file \"%s\" failed.\r\n", session->precmd.arg);
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "250 Rename file \"%s\" to \"%s\" succeeded!\r\n", session->precmd.arg, session->command.arg);
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command DELE */
/****************/

int handleCmdDele(Session* session) {
    char resBuf[100];

    int res = remove(session->command.arg);
    if (res < 0) {
        sprintf(resBuf, "550 Command DELE failed.\r\n");
        sendResp(session->ctrlSock, resBuf);
        return 0;
    }

    sprintf(resBuf, "250 Okay.\r\n");
    sendResp(session->ctrlSock, resBuf);
    return 1;
}


/****************/
/* command QUIT */
/****************/

int handleCmdQuit(Session* session) {
    char resBuf[1024];
    // show(session);
    sprintf(resBuf, "221-You have downloaded %lld bytes in %d files.\r\n221-And uploaded %lld bytes in %d files.\r\n221-Thank you for using the FTP service.\r\n221 Goodbye.\r\n",
            session->down_size, session->down_files, session->up_size, session->up_files);
    sendResp(session->ctrlSock, resBuf);
    session->quitFlag = 1;
    return 1;
}


/*******************/
/* command UNKNOWN */
/*******************/

int handleCmdUnkn(Session* session) {
    char* resBuf = "500 Syntax error, command unrecognized.\r\n";
    sendResp(session->ctrlSock, resBuf);
    return 0;
}
