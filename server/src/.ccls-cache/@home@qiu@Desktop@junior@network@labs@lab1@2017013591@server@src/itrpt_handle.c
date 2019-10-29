#include "itrpt_handle.h"

void handleInterrupt(int signal) {
    switch (signal) {
        case SIGINT:
            break;
        case SIGTERM:
            printf("server received SIGTERM.\n");
            printStack();
            break;
        case SIGQUIT:
            printf("server received SIGQIOT.\n");
            printStack();
            break;
        case SIGSEGV:
            printf("server received SIGSEGV.\n");
            printStack();
            break;
        case SIGPIPE:
            printf("server received SIGPIPE.\n");
            printStack();
            break;
        case SIGABRT:
            printf("server received SIGABRT.\n");
            printStack();
            break;
        case SIGKILL:
            printf("server received SIGKILL.\n");
            printStack();
            break;
    }
    closeAllSockets();
    exit(1);
}


void closeAllSockets() {
    close(listenfd);
    clear(&sessions);
}

void printStack() {
    void *trace[32];
    int size = backtrace(trace, 32);
    char **symbols = (char **) backtrace_symbols(trace, size);
    for (int i = 0; i < size; i++) {
        printf("%d--->%s\n", i, symbols[i]);
    }
}
