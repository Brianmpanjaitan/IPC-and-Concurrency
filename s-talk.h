#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "list.h"

#define MAX_LEN 1024

typedef struct l_info
{
	List *list;
	struct addrinfo *serv, *host;
} l_info;

void* keyb_input(void *listsend);

void* send_data(void *args_ptr);

void* recv_data(void *args_ptr);

void* print_screen(void *listrecv);

int main(int argc, char *argv[]);
