#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

std::string work(ssize_t len){
	if (len % 2 == 0){
		return "Winner winner chicken dinner!";
	}
	else {
		return "Good luck next time!";
	}
}

int main(int argc, const char* argv[]){
	if (argc != 2){
		perror("Expected 1 argument for address");
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
	serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	int Bind = bind(socketDescriptor, (struct sockaddr *) &serverInfo, sizeof(serverInfo));
	if (Bind == -1){
		perror("Bind failed");
		return 1;
	}
	int listener = listen(socketDescriptor, 1);
	if (listener == -1){
		perror("Listen failed");
		return 1;
	}
	printf("Waiting for connection\n");
	while (true){
		int clientDescriptor = accept(socketDescriptor, NULL, NULL);
		if (clientDescriptor == -1){
			perror("Accept failed");
			return 1;
		}
		while (true){
			ssize_t rec_count = 0;
			//fix for incorrect recv work
			while (true){
				ssize_t tmp = recv(clientDescriptor, msg + rec_count, 1024, 0);
				if (tmp <= 0){
					break;
				}
				rec_count += tmp;
				if (msg[rec_count - 1] == '\0'){
					break;
				}
			}
			if (rec_count <= 0){
				break;
			}
			msg[rec_count - 1] = '\0';
			std::string a = work(strlen(msg));
			char ans[a.length() + 1];
			for (size_t i = 0; i < a.length(); i++){
				ans[i] = a[i];
			}
			size_t cnt = 0;
			while (cnt != strlen(ans)){
				ssize_t tmp = send(clientDescriptor, ans + cnt, strlen(ans) - cnt, 0);
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
				tmpCount = send(clientDescriptor, strend, 1, 0);
			}
		}
		close(clientDescriptor);
	}
}