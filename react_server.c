/*
 *  Operation Systems (OSs) Course Assignment 4
 *  Reactor - A TCP server that handles multiple clients using a reactor.
 *  Copyright (C) 2023  Roy Simanovich and Linor Ronen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "reactor.h"

reactor_t_ptr reactor = NULL;

int main(void) {
	struct sockaddr_in server_addr;

	int server_fd = -1, reuse = 1;

	signal(SIGINT, signal_handler);

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket() failed");
		return 1;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
		close(server_fd);
		return 1;
	}

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind() failed");
		close(server_fd);
		return 1;
	}

	if (listen(server_fd, MAX_QUEUE) < 0)
	{
		perror("listen() failed");
		close(server_fd);
		return 1;
	}

	fprintf(stdout, "[INFO] Server listening on port %d\n", SERVER_PORT);

	reactor = createReactor();

	if (reactor == NULL)
	{
		fprintf(stderr, "createReactor() failed: memory allocation error\n");
		return 1;
	}

	fprintf(stdout, "[INFO] Adding server socket to reactor...\n");
	addFd(reactor, server_fd, server_handler);
	fprintf(stdout, "[INFO] Server socket added to reactor\n");

	startReactor(reactor);
	WaitFor(reactor);

	signal_handler();

	return 0;
}

void signal_handler() {
	fprintf(stdout, "[INFO] Server shutting down...\n");
	
	if (reactor != NULL)
	{
		stopReactor(reactor);

		fprintf(stdout, "[INFO] Closing all sockets and freeing memory...\n");

		reactor_node_ptr curr = reactor->head;
		reactor_node_ptr prev = NULL;

		while (curr != NULL)
		{
			prev = curr;
			curr = curr->next;

			close(prev->fd);
			free(prev);
		}

		free(reactor);

		fprintf(stdout, "[INFO] Memory cleanup complete, may the force be with you.\n");
	}

	else
		fprintf(stdout, "[INFO] Reactor wasn't created, no memory cleanup needed.\n");

	exit(0);
}

void *client_handler(int fd, void *arg) {
	char buf[MAX_BUFFER] = { 0 };

	int bytes_read = recv(fd, buf, sizeof(buf), 0);

	if (bytes_read < 0)
	{
		fprintf(stderr, "[ERROR] Failed to read from client %d\n", fd);
		close(fd);
		return NULL;
	}

	else if (bytes_read == 0)
	{
		fprintf(stdout, "[INFO] Client %d disconnected\n", fd);
		close(fd);
		return NULL;
	}

	// Make sure the buffer is null-terminated, so we can print it.
	buf[bytes_read] = '\0';

	fprintf(stdout, "[MESSAGE] Client %d: %s\n", fd, buf);

	return arg;
}

void *server_handler(int fd, void *arg) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	reactor_t_ptr reactor = (reactor_t_ptr)arg;

	if (reactor == NULL)
	{
		fprintf(stderr, "Server handler error: reactor is NULL.\n");
		return NULL;
	}

	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);

	if (client_fd < 0)
	{
		fprintf(stderr, "[ERROR] Failed to accept client\n");
		return NULL;
	}

	addFd(reactor, client_fd, client_handler);

	fprintf(stdout, "[INFO] Client %s:%d connected, ID: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

	return arg;
}