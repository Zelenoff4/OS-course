#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, const char* argv[]){
	if (argc != 3){
		perror("Expected 2 arguments: address and a name");
		return 1;
	}
	int socketDescriptor;
	char msg[1024];
	struct sockaddr_in serverInfo;
	socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (socketDescriptor == -1){
		perror("Socket failed");
		return 1;
	}
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(2590);
	serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	int Connector = connect(socketDescriptor, (struct sockaddr *) &serverInfo, sizeof(serverInfo));
	if (Connector == -1){
		perror("Connect failed");
		return 1;
	}
	size_t cnt = 0;
	while(cnt != strlen(argv[2])){
		ssize_t tmp = send(socketDescriptor, argv[2] + cnt, strlen(argv[2]) - cnt, 0);
		if (tmp == -1){
			perror("Send failed");
			return 1;
		}
		cnt += tmp;
	}
	//fix the '\0' problem
	ssize_t tmpCount = 0;
	char strend[1] = {'\0'};
	while (tmpCount != 1){
		tmpCount = send(socketDescriptor, strend, 1, 0);
	}
	ssize_t recvCount = 0;
	while (true){
		ssize_t tmp = recv(socketDescriptor, msg + recvCount, 1024, 0);
		if (tmp == -1){
			perror("Recieve failed");
			return 1;
		}
		recvCount += tmp;
		if (msg[recvCount - 1] == '\0'){
			break;
		}
	}
	printf("%s\n", msg);
	close(socketDescriptor);
	return 0;
}
