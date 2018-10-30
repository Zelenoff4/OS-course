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
	socketDescriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (socketDescriptor == -1){
		perror("Socket failed");
		return 1;
	}
	struct sockaddr_in serverInfo;
	int optval = 1;
	int setSockInfo = setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (setSockInfo == -1){
		perror("setsockopt failed");
		close(socketDescriptor);
		return 1;
	}
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(2590);
	serverInfo.sin_addr.s_addr = htonl(INADDR_ANY);
	int Bind = bind(socketDescriptor, (struct sockaddr *) &serverInfo, sizeof(serverInfo));
	if (Bind == -1){
		perror("Bind failed");
		close(socketDescriptor);
		return 1;
	}
	int listener = listen(socketDescriptor, 1);
	if (listener == -1){
		perror("Listen failed");
		close(socketDescriptor);
		return 1;
	}
	int epollDescriptor = epoll_create(100);
	if (epollDescriptor == -1){
		perror("Epoll_create failed");
		close(socketDescriptor);
		return 1;
	}
	static struct epoll_event event, maxEvents[100];
	event.data.fd = socketDescriptor;
	event.events = EPOLLIN;
	int epollctl = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, socketDescriptor, &event);
	if (epollctl == -1){
		perror("Epoll_ctl failed");
		close(socketDescriptor);
		close(epollDescriptor);
		return 1;
	}
	printf("Waiting for connection\n");
	while (true) {
		int waiter = epoll_wait(epollDescriptor, maxEvents, 100, 1000000);
		if (waiter == -1){
			perror("Epoll_wait failed");
			close(socketDescriptor);
			close(epollDescriptor);
			return 1;
		}
		for (int i = 0; i < waiter; i++){
			if (socketDescriptor == maxEvents[i].data.fd){
				struct sockaddr_in clientInfo;
				socklen_t clientLen = sizeof(clientInfo);
				int clientDescriptor = accept(socketDescriptor, (struct sockaddr *) &clientInfo, &clientLen);
				if (clientDescriptor == -1){
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK)){
						continue;
					}
					else {
						perror("Accept failed");
						break;
					}
				}
				static struct epoll_event clientEvent;
				clientEvent.data.fd = clientDescriptor;
				clientEvent.events = EPOLLIN | EPOLLRDHUP;
				epollctl = epoll_ctl(epollDescriptor, EPOLL_CTL_ADD, clientDescriptor, &clientEvent);
				if (epollctl == -1){
					perror("Epoll_ctl failed");
					return 1;
				}
			}
			else{
				if (maxEvents[i].events == EPOLLIN){
					int clientDescriptor = maxEvents[i].data.fd;
					ssize_t rec_count = 0;
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
					ssize_t tmpCount = 0;
					char strend[1] = {'\0'};
					while (tmpCount != 1){
						tmpCount = send(clientDescriptor, strend, 1, 0);
					}
				}
				else {
					close(maxEvents[i].data.fd);
				}
			}
		}
	}
	close(socketDescriptor);
	close(epollDescriptor);
	return 0;

}