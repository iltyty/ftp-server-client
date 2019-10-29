#include "init_socket.h"

int initListenSocket(int port) {
    int listenfd;
    struct sockaddr_in addr;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd == -1) {
        char err[200];
        sprintf(err, "Error socket(): %s(%d)\n", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int n = 1;
    if ((n = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(int))) < 0) {
        print_log("Received error", "Error setsocket option.");
    }

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        char err[200];
        sprintf(err, "Error bind(): %s(%d)\n", strerror(errno), errno);
        print_log("Received error", err);

        return -1;
    }

    if (listen(listenfd, 10) == -1) {
        char err[200];
        sprintf(err, "Error listen(): %s(%d)\n", strerror(errno), errno);
        print_log("Received error", err);
        return -1;
    }

    // char host[100];
    // printf("---server 127.0.0.1 running on port: %d---\n", port);
    char cont[200], host[100];
    getIpAddress(host);
    sprintf(cont, "Listening on %s:%d", host, port);
    print_log("Server started", cont);
    /* getIpAddress(host);                                          */
    /* printf("---server %s running on port: %d---\n", host, port); */

    return listenfd;
}
