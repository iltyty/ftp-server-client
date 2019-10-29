#include "data_struct.h"

void strrpl(char *str, char srcch, char dstch) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == srcch) {
            str[i] = dstch;
        }
    }
}


bool assignCmd(Command* dst, Command* src) {
    dst->cmd = src->cmd;

    if (dst->arg) {
        free(dst->arg);
    }

    if (src->arg) {
        dst->arg = (char *) malloc(sizeof(char) * (strlen(src->arg) + 1));
        if (!dst->arg) {
            printf("Error assign command %d to command %d: malloc error.\n", src->cmd, dst->cmd);
            return false;
        }
        strcpy(dst->arg, src->arg);
    } else {
        dst->arg = NULL;
    }

    return true;
}


bool init(SList* slist) {
    slist->head = (Session*) malloc(sizeof(Session));
    slist->tail = slist->head;
    if (!slist->head) {
        printf("Error initializing session list!\n");
        return false;
    }

    slist->head->next = NULL;
    slist->size = 0;
    return true;
}


bool add(SList* slist, SNode snode) {
    if (!snode) {
        printf("Error add session node of null!\n");
        return false;
    }

    slist->tail->next = snode;
    slist->tail = snode;
    slist->size++;
    return true;
}



bool del(SList* slist, int id) {
    if (slist->size == 0) {
        printf("Error delete id %d: session list empty!\n", id);
        return false;
    }

    SNode p = get(slist, id);
    if (!p) {
        printf("Error delete id %d: id does not exist!\n", id);
        return false;
    }

    if (id == slist->tail->id) {
        return pop(slist);
    }

    SNode q = p->next;
    assign(p, q);
    p->next = q->next;
    closeSession(q);
    free(q);
    slist->size--;
    return true;
}


bool pop(SList* slist) {
    if (slist->size == 0) {
        printf("Error pop: session list empty!\n");
        return false;
    }

    SNode p = slist->head;
    while (p && p->next != slist->tail) {
        p = p->next;
    }

    closeSession(slist->tail);
    free(slist->tail);
    slist->tail = p;
    slist->tail->next = NULL;
    slist->size--;
    return true;
}


SNode get(SList* slist, int id) {
    SNode p = slist->head->next;
    while (p && p->id != id) {
        p = p->next;
    }
    return p;
}


SNode find(SList* slist, int socketfd) {
    SNode p = slist->head->next;
    while (p && p->ctrlSock != socketfd && p->dataSock != socketfd) {
        p = p->next;
    }
    return p;
}


bool clear(SList* slist) {
    if (slist->size == 0) {
        return true;
    }

    SNode p = slist->head->next;
    while (p) {
        slist->head->next = p->next;
        closeSession(p);
        free(p);
        p = slist->head->next;
    }

    slist->tail = slist->head;
    slist->size = 0;
    return true;
}


bool assign(SNode dst, SNode src) {
    dst->id = src->id;
    dst->ctrlSock = src->ctrlSock;
    dst->dataSock = src->dataSock;

    free(dst->client.username);
    free(dst->client.password);
    dst->client.loggedin = src->client.loggedin;
    dst->client.username = (char*) malloc(sizeof(char) * strlen(src->client.username));
    dst->client.password = (char*) malloc(sizeof(char) * strlen(src->client.password));
    if (!dst->client.username || !dst->client.password) {
        printf("Error assign session %d to session %d:, memory allocation failed!\n", dst->id, src->id);
        return false;
    }
    strcpy(dst->client.username, src->client.username);
    strcpy(dst->client.password, src->client.password);

    free(dst->command.arg);
    dst->command.arg = (char*) malloc(sizeof(char) * strlen(src->command.arg));
    if (!dst->command.arg) {
        printf("Error assign session %d to session %d:, memory allocation failed!\n", dst->id, src->id);
        return false;
    }
    strcpy(dst->command.arg, src->command.arg);
    dst->command.cmd = src->command.cmd;

    dst->dataAddr = src->dataAddr;
    return true;
}


bool closeSession(Session* session) {
    close(session->ctrlSock);
    close(session->dataSock);
    return true;
}

void show(Session* session) {
    printf("\tid: %d\t\n", session->id);
    printf("\tcontrol socket: %d\t\n", session->ctrlSock);
    printf("\tclient username: %s\t\n", session->client.username);
    printf("\ttotal downloaded file numbers: %d\t\n", session->down_files);
    printf("\ttotal downloaded file size: %lld\t\n", session->down_size);
    printf("\ttotal uploaded file numbers: %d\t\n", session->up_files);
    printf("\ttotal uploaded file size: %lld\t\n", session->up_size);
    printf("\tcommand: %d %s\t\n", session->command.cmd, session->command.arg);
}
