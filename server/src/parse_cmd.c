#include "parse_cmd.h"


COMMAND dict(char* cmd) {
    for (int i = 0; i < 20; i++) {
        if (!strcmp(cmd, cmd_const[i])) {
            return i;
        }
    }
    return UNKNOWN;
}


int parseCommand(char* recBuf, Session *session) {
    trim(recBuf, " \r\n\t\f");

    int len = strlen(recBuf);
    Command *command = &(session->command);

    // emtpy command
    if (len == 0) {
        return CMD_EMPTY_ERROR;
    }

    char *sep = " ";
    char **result = (char**) malloc(sizeof(char*) * 10);
    for (int i = 0; i < 10; i++) {
        result[i] = (char*) malloc(sizeof(char) * 50);
    }
    split(recBuf, sep, result);

    int count = 0;
    while (result[count]) {
        count++;
    }

    command->cmd = dict(result[0]);
    bool flag = argRequired(*command);

    if (command->cmd == UNKNOWN) {
        return CMD_EMPTY_ERROR;
    }

    if ((flag && count != 2) || (!flag && count != 1)) {
        return ARG_SYNTAX_ERROR;
    }

    if (loginRequired(*command) && !session->client.loggedin) {
        return LOGIN_REQUIRED_ERROR;
    }

    if (flag) {
        command->arg = (char *) malloc(sizeof(char) * (strlen(result[1]) + 1));
        strcpy(command->arg, result[1]);
    } else {
        command->arg = NULL;
    }

    for (int i = 0; i < 10; i++) {
        free(result[i]);
    }
    free(result);


    return 1;
}


bool loginRequired(Command command) {
    switch (command.cmd) {
        case USER:
        case PASS:
        case ABOR:
        case QUIT:
        case SYST:
            return false;
        default:
            return true;
    }
}


bool argRequired(Command command) {
    switch (command.cmd) {
        case MKD:
        case CWD:
        case RMD:
        case DELE:
        case USER:
        case PASS:
        case RETR:
        case STOR:
        case TYPE:
        case PORT:
        case REST:
        case RNFR:
        case RNTO:
            return true;
        default:
            return false;
    }
}


bool in(char ch, char *delim) {
    for (int i = 0; i < strlen(delim); i++) {
        if (ch == delim[i]) {
            return true;
        }
    }

    return false;
}


void trim(char* str, char* delim) {
    char* tmp1 = str;
    while (in(*str, delim)) {
        str++;
    }
    if (*str) {
        char* tmp2 = str;
        while (*str) {
            str++;
        }
        str--;
        while (in(*str, delim)) {
            str--;
        }
        while (tmp2 <= str) {
            *(tmp1++) = *(tmp2++);
        }
    }
    *tmp1 = 0;
}


void split(char *source, char *sep, char **result) {
    char *p;
    int n = 0;

    p = strtok(source, sep);
    while (p) {
        strcpy(result[n++], p);
        p = strtok(NULL, sep);
    }
    result[n] = NULL;
}
