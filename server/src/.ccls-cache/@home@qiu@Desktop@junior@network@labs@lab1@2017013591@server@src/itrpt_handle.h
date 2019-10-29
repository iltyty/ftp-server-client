#ifndef _ITRPT_HANDLE_H
#define _ITRPT_HANDLE_H

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>

#include "data_struct.h"

extern int listenfd;
extern SList sessions;

extern void handleInterrupt(int signal);
extern void closeAllSockets();
extern void printStack();

#endif
