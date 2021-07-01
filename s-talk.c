#include "s-talk.h"
#include "list.h"

struct addrinfo hints, *server_info, *local;
int sockfd, newfd;
int flag = 1;
char sendline[MAX_LEN];
char recvline[MAX_LEN];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t okToSend = PTHREAD_COND_INITIALIZER;
pthread_cond_t okToPrint = PTHREAD_COND_INITIALIZER;

// pthread to await input from the keyboard
void* keyb_input(void *listsend)
{
	//fgets(sendline, MAX_LEN, stdin);
	while(flag)
	{
        fgets(sendline, MAX_LEN, stdin);
		if(strcmp(sendline, "!\n") == 0)
		{
			printf("Ending session\n");
			flag = 0;
			break;
		}
		pthread_mutex_lock(&mutex);

		List_append((List*)listsend, sendline);

		pthread_cond_signal(&okToSend);
        pthread_mutex_unlock(&mutex);
	}
	pthread_cond_signal(&okToSend);
    pthread_cond_signal(&okToPrint);
	bzero(sendline, MAX_LEN);
	
	pthread_exit(&flag);
}

// pthread to send data to the remote UNIX process
void* send_data(void *args_ptr)
{
    struct l_info *sendptr = (struct l_info*)args_ptr;
	char *msg;

	while(flag)
	{
		pthread_mutex_lock(&mutex);
        pthread_cond_wait(&okToSend, &mutex);

		if(flag)
		{
			List_last(sendptr->list);
			msg =(char*)List_remove(sendptr->list);
			sendto(sockfd, msg, MAX_LEN, 0, server_info->ai_addr, server_info->ai_addrlen);
		}
		pthread_mutex_unlock(&mutex);
	}
	free(msg);
    pthread_exit(&flag);
}

// pthread to await a UDP datagram
void* recv_data(void *args_ptr)
{
    struct l_info *recvptr = (struct l_info*)args_ptr;
    
	struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int res;
	while(flag)
	{
		if(res = recvfrom(sockfd, recvline, MAX_LEN, 0, (struct sockaddr*)&from, &fromlen) > 0)
		{
            pthread_mutex_lock(&mutex);
			List_append(recvptr->list, recvline);
			bzero(sendline, MAX_LEN);
			
			pthread_cond_signal(&okToPrint);
            pthread_mutex_unlock(&mutex);
		}
	}
	bzero(sendline, MAX_LEN);
    pthread_exit(&flag);
}	

// pthread to print characters to screen
void* print_screen(void *listrecv)
{	
	while(flag)
	{
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&okToPrint, &mutex);
                
		if(flag)
		{
			List_first(listrecv);		    
			printf("Them: %s", (char*)List_remove((List*)listrecv));
		}
		pthread_mutex_unlock(&mutex);
	}
    pthread_exit(&flag);
}


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("No port\n");
		return -1;
	}
    printf("session with port %s...\n", argv[1]);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
	{
		printf("socket creation failed\n");
		return -1;
	}
	int status;
	
	// Create socket
	memset(&hints, 0, sizeof(hints)); 	// Make struct empty
	hints.ai_family = AF_UNSPEC;		// for IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP stream sockets
	hints.ai_flags = AI_PASSIVE;		// fill in IP
	hints.ai_protocol = 0;
	
	// Info N/A
	if((status = getaddrinfo(NULL, argv[1], &hints, &local) != 0))
	{
		return -1;
	}
	
	// Bind
	if(bind(sockfd, local->ai_addr, local->ai_addrlen) < 0)
	{
		printf("bind failed");
		return -1;
	}
	
	// Store local
	getaddrinfo(NULL, argv[1], &hints, &local); 
	
	// Store connection
	getaddrinfo(argv[2], argv[3], &hints, &server_info); 
	
  	struct l_info send_l, recv_l; //struct for send and recv list
    send_l.list = List_create();
   	recv_l.list = List_create();
	
	pthread_t keyb_t, screen_t, send_t, recv_t;
	
	pthread_create(&keyb_t, NULL, keyb_input, (void*)send_l.list); // Pass in list
	pthread_create(&send_t, NULL, send_data, &send_l); // Pointer to struct
	pthread_create(&recv_t, NULL, recv_data, &recv_l); // Pointer to struct
	pthread_create(&screen_t, NULL, print_screen, (void*)recv_l.list); // Pass in list
 
	pthread_join(keyb_t, NULL);
	pthread_join(send_t, NULL);
	pthread_join(recv_t, NULL);	
	pthread_join(screen_t, NULL);
	return 0;
}
