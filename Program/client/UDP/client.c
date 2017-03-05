#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXD 4096
#define N 64

typedef struct {
	char *Server_addr;
	int Port;
} Server_arg;

int run = 1;
int pos = N/2;
char TableBuf[N*N];
char Table[N*(N+1)];

void *Render(){
#define Vert(x, y) ((x)*(N+1)+(y))
	while(run){
		usleep(100000);
		for(int x=0; x<N; x++){
			for(int y=0; y<N; y++){
				char tmp;
				if(x==0 || x==N-1 || y==0 || y==N-1)
					tmp = '=';
				else if (TableBuf[x*N+y]==0)
					tmp = ' ';
				else
					tmp = '*';
				Table[Vert(x, y)] = tmp;
			}
			Table[Vert(x, N)] = '\n';
		}
		Table[Vert(N-1, N)] = '\0';
		puts(Table);
	}
	pthread_exit(NULL);
#undef Vert
}

void *SendMsg(void *connect_arg){
	Server_arg *server_arg = (Server_arg *)connect_arg;
	struct sockaddr_in server_addr;
	int sock_out = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_out < 0){
		perror("Opening the datagram socket error");
		exit(1);
	}
	
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_arg->Server_addr);
	server_addr.sin_port = htons(server_arg->Port);

	puts("Client connects to the server.");
	char databuf[MAXD];
	while (run){
		usleep(100000);
		sprintf(databuf, "%d", pos);
		if (sendto(sock_out, databuf, sizeof(databuf), 0,
					(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
			perror("Send Msg to server failed");
			continue;
		}
	}
	pthread_exit(NULL);
}

void *RecvMsg(void *connect_arg){
	Server_arg *server_arg = (Server_arg *)connect_arg;
	struct sockaddr_in server_addr, client_addr;
	int sock_in = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_in < 0){
		perror("Opening the datagram socket error");
		exit(1);
	}

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_arg->Port);

	if (bind(sock_in, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
		perror("server socket binding error");
		exit(1);
	}

	printf("Client starts listening on the port %d.\n", server_arg->Port);
	int len = sizeof(client_addr);
	char databuf[MAXD];
	while (run){
		if (recvfrom(sock_in, databuf, sizeof(databuf), 0,
				(struct sockaddr *)&client_addr, (socklen_t *)&len) < 0){
			perror("Receive Msg error");
			continue;
		}
		else
			memcpy(TableBuf, databuf, N*N);
	}
	pthread_exit(NULL);
}

int main (int argc, char *argv[]){
	if (argc != 3){
		puts("usage: ./client [SERVER_IP_ADDR] [PORT]");
		exit(1);
	}

	Server_arg server_arg;
	server_arg.Server_addr = argv[1];
	server_arg.Port = atoi(argv[2]);

	pthread_t sendThread, recvThread, renderThread;
	if (pthread_create(&sendThread, NULL, SendMsg, (void *)&server_arg) < 0)
		puts("Sending thread creation failed.");
	if (pthread_create(&recvThread, NULL, RecvMsg, (void *)&server_arg) < 0)
		puts("Receving thread creation failed.");
	if (pthread_create(&renderThread, NULL, Render, (void *)NULL) < 0)
		puts("Render thread creation failed.");

	int instruction;
	while(scanf("%d", &instruction) == 1){
		if(instruction == 1)
			pos --;
		else if(instruction == 2)
			pos ++;
		else{
			run = 0;
			break;
		}
	}

	pthread_join(sendThread, NULL);
	pthread_join(recvThread, NULL);
	pthread_join(renderThread, NULL);
	return 0;
}
