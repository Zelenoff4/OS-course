#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>

int main(int argc, const char* argv[]){
	if (argc != 2){
		perror("Expected 1 argument for address");
		return 1;
	}
	int socketDescriptor;
	char msg[1024];
	struct sockaddr_in serverInfo;
	socketDescriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketDescriptor == -1){
		perror("Socket failed");
		return 1;
	}
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(2590);
	serverInfo.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	int Connector = connect(socketDescriptor, (struct sockaddr *) &serverInfo, sizeof(serverInfo));
	static struct epoll_event event, maxEvents[100];
	int epollDescriptor = epoll_create(100);
	if (epollDescriptor == -1){
		perror("Epoll_create failed");
		close(socketDescriptor);
		return 1;
	}
	bool isConnected = true;
	if ((Connector == -1) && (errno == EINPROGRESS)){
		isConnected = false;
	}
	event.events = EPOLLIN;
	event.data.fd = STDIN_FILENO;
	int epollctl = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, STDIN_FILENO, &event);
	if (epollctl == -1){
		perror("Epoll_ctl failed");
		close(socketDescriptor);
		close(epollDescriptor);
		return 1;
	}
	event.events = EPOLLIN;
	if (!isConnected){
		event.events |= EPOLLOUT;
	}
	event.data.fd = socketDescriptor;
	epollctl = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, socketDescriptor, &event);
	if (epollctl == -1){
		perror("Epoll_ctl failed");
		close(socketDescriptor);
		close(epollDescriptor);
		return 1;
	}
	bool cycle = true;
	while (cycle){
		int waiter = epoll_wait(epollDescriptor, maxEvents, 100, 1000000);
		if (waiter == -1){
			perror("Epoll_wait failed");
			close(socketDescriptor);
			close(epollDescriptor);
			return 1;
		}
		for (int i = 0; i < waiter; i++){
			if (socketDescriptor == maxEvents[i].data.fd && (maxEvents[i].events & EPOLLOUT) && !isConnected){
				int tmp = 0;
				socklen_t len = sizeof(int);
				int val = getsockopt(socketDescriptor, SOL_SOCKET, SO_ERROR, &tmp, &len);
				if (val != -1){
					if (tmp == 0){
						event.events = EPOLLIN;
						event.data.fd = socketDescriptor;
						epollctl = epoll_ctl(epollDescriptor, EPOLL_CTL_MOD, socketDescriptor, &event);
						if (epollctl == -1){
							perror("Epoll_ctl failed");
							close(socketDescriptor);
							close(epollDescriptor);
							return 1;
						}
					}
					else {
						perror("Connection failed");
						event.events = 0;
						event.data.fd = socketDescriptor;
						epoll_ctl(epollDescriptor, EPOLL_CTL_DEL, socketDescriptor, &event);
						close(socketDescriptor);
					}
				}
			}
			else {
				if (maxEvents[i].data.fd == STDIN_FILENO){
					scanf("%s", msg);
					std::string check = msg;
					if (check == "exit"){
						cycle = false;
						break;
					}
					size_t cnt = 0;
					while(cnt != strlen(msg)){
						ssize_t tmp = send(socketDescriptor, msg + cnt, strlen(msg) - cnt, 0);
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
				}
				else {
					if (maxEvents[i].data.fd == socketDescriptor){
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
					}
				}
			}
		}
	}
	close(socketDescriptor);
	close(epollDescriptor);
	return 0;
}