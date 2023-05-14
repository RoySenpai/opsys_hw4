#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "reactor.h"

void *client_handler(int fd, void *arg) {
	char buf[1024];

	int bytes_read = recv(fd, buf, sizeof(buf), 0);

	buf[bytes_read] = '\0';

	printf("Client %d: %s\n", fd, buf);
}

void *server_handler(int fd, void *arg) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	addFd(arg, client_fd, client_handler);
}

int main(int argc, char **argv) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd == -1)
	{
		printf("Failed to create socket\n");
		return 1;
	}

	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("Failed to bind socket\n");
		return 1;
	}

	if (listen(server_fd, LISTENQ) < 0)
	{
		printf("Failed to listen on socket\n");
		return 1;
	}

	reactor_t_ptr reactor = createReactor();

	if (reactor == NULL)
	{
		printf("Failed to create reactor\n");
		return 1;
	}

	addFd(reactor, server_fd, server_handler);

	startReactor(reactor);

	WaitFor(reactor);

	free(reactor);

	close(server_fd);

	return 0;
}