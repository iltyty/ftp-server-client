#include <getopt.h>

#include "init_socket.h"
#include "handle_client.h"

SList sessions;
int listenfd;

#define PORT 21

static struct option long_opts[] = {
    {"port", required_argument, NULL, ARG_PORT},
    {"root", required_argument, NULL, ARG_ROOT},
    {NULL, 0, NULL, 0},
};

void parse_args(int argc, char *argv[], int *listenfd);

int main(int argc, char *argv[]) {
    signal(SIGINT,  handleInterrupt);
    signal(SIGABRT, handleInterrupt);
    signal(SIGTERM, handleInterrupt);
    signal(SIGQUIT, handleInterrupt);
    // signal(SIGPIPE, handleInterrupt);
    signal(SIGPIPE, SIG_IGN);
    // signal(SIGSEGV, handleInterrupt);
    signal(SIGKILL, handleInterrupt);

    init(&sessions);

    parse_args(argc, argv, &listenfd);
    if (listenfd < 0) {
        printf("Error initializing listen socket\n");
        return -1;
    }

    while (true) {
        Session* session = (Session*) malloc(sizeof(Session));

        int connfd = getClientSocket(listenfd);
        if (connfd < 0) {
            continue;
        }

        // add(&sessions, session);
        // session->id = sessions.size;
        session->ctrlSock = connfd;
        session->dataSock = 0;
        session->pos = 0;
        session->quitFlag = false;

        pid_t pid = fork();
        if (pid == 0) {
            close(listenfd);
            srand(time(0) % getpid());
            dealWithClient(session);
            return 0;
        } else if (pid > 0) {
            close(connfd);
            continue;
        } else {
            printf("fork fail\n");
            continue;
        }

    }

    close(listenfd);
}

void parse_args(int argc, char *argv[], int *listenfd) {
    int opt_port;
    char *opt_root;
    char *opt_string = "";
    int opt, opt_index = 0;
    bool port = false, root = false;

    while ((opt = getopt_long_only(argc, argv, opt_string, long_opts, &opt_index)) != -1) {
        printf("%c", opt);
        switch (opt) {
            case ARG_PORT:
                port = true;
                opt_port = atoi(optarg);
                break;
            case ARG_ROOT:
                root = true;
                opt_root = optarg;
                break;
            case '?':
                printf("Unrecognized arg.\n");
                exit(-1);
            default:
                break;
        }
    }

    if (port) {
        *listenfd = initListenSocket(opt_port);
    } else {
        *listenfd = initListenSocket(PORT);
    }

    if (root) {
        chdir(opt_root);
    } else {
        chdir("/tmp");
    }
}
