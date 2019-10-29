#ifndef _INIT_SOCKET_H
#define _INIT_SOCKET_H

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "handle_client.h"

// create listening socket
int initListenSocket(int port);

#endif
