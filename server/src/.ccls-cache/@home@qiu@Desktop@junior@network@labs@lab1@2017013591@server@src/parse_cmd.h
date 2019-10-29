#ifndef _PARSE_CMD_H
#define _PARSE_CMD_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data_struct.h"


COMMAND dict(char* cmd);
int  cmdLegal(Command command);
bool in(char ch, char *delim);
void trim(char* str, char *delim);
bool argRequired(Command command);
bool loginRequired(Command command);
int  parseCommand(char* recBuf, Session *session);
void split(char *source, char *sep, char **result);

#endif
