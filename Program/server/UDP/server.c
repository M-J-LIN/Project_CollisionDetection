#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAXD 4096
#define MAXU 16
#define N 64

typedef struct {
	char *client_addr;
} Client;

Client client[MAXU];
int nClient = 0;

char Table[N][N] = {0};
char Msg[MAXU][MAXD] = {0};

void *SendMsg(void *port){
	int PORT = atoi((char *)port);
	struct sockaddr_in client_addr;
	int sock_out = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (sock_out < 0){
		perror("Opening the datagram socket error");
		exit(1);
	}

	char databuf[MAXD];
	while (1){
		sleep(1);
		memcpy(databuf, Table, sizeof(char)*N*N);
		for (int i=0; i<nClient; i++){
			memset((char *)&client_addr, 0, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			client_addr.sin_addr.s_addr = inet_addr(client[i].client_addr);
			client_addr.sin_port = htons(PORT);

			if (sendto(sock_out, databuf, sizeof(databuf), 0,
					(struct sockaddr*)&client_addr, sizeof(client_addr)) < 0){
				printf("Send Msg to %d user failed.\n", i);
				continue;
			}
//			else
//				puts("Send Msg success.");
		}
	}

	pthread_exit(NULL);
}

void *RecvMsg(void *port){
	int PORT = atoi((char *)port);
	struct sockaddr_in server_addr, client_addr;
	int sock_in = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_in < 0){
		perror("Opening the datagram socket error");
		exit(1);
	}

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	if (bind(sock_in, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		perror("server socket binding error");
		exit(1);
	}

	printf("Server starts listening on the port %d.\n", PORT);
	int len = sizeof(client_addr);
	char databuf[MAXD];

	while(1){
		if (recvfrom(sock_in, databuf, sizeof(databuf), 0,
				(struct sockaddr*)&client_addr, (socklen_t *)&len) < 0){
			perror("Receive Msg error");
			continue;
		}

		char *tmp_addr = inet_ntoa(client_addr.sin_addr);
//		printf("Receive data from %s : %d\n", tmp_addr, htons(client_addr.sin_port));

		int isNew = 1;
		for (int i=0; i<nClient && isNew; i++){
			if(memcmp(client[i].client_addr, tmp_addr, 15)==0){
				memcpy(Msg[i], databuf, MAXD);
				isNew = 0;
			}
		}
		if (isNew){
			client[nClient].client_addr = (char *)malloc(sizeof(char)*16);
			memcpy(client[nClient].client_addr, tmp_addr, 15);
			nClient++;
		}
	}
	pthread_exit(NULL);	
}

int main(int argc, char *argv[]){
	if (argc != 2){
		puts("usage : ./server [PORT]");
		exit(1);
	}

	pthread_t sendThread, recvThread;
	if(pthread_create( &sendThread, NULL, SendMsg, (void *)argv[1]) < 0)
		puts("Sending thread creation failed.");
	if (pthread_create( &recvThread, NULL, RecvMsg, (void *)argv[1]) < 0)
		puts("Receving thread creation failed.");

	while(1){
		for(int i=0; i<MAXU; i++){
			if(Msg[i][0] != '\0'){
				int tmp_pos = atoi(Msg[i]);
				if(tmp_pos > 0 && tmp_pos < N-1){
					memset(Table[i+1], 0, sizeof(char)*N);
					Table[i+1][tmp_pos] = '*';
				}
			}
			Msg[i][0] = '\0';
		}
	}

	pthread_join(sendThread, NULL);
	pthread_join(recvThread, NULL);
	return 0;
}
