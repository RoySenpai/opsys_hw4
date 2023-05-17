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

#include "reactor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *reactorRun(void *react) {
	if (react == NULL)
	{
		errno = EINVAL;
		perror("\033[0;31m[ERROR]\033[0;37m reactorRun() failed");
		return NULL;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	while (reactor->running)
	{
		size_t size = 0, i = 0;
		reactor_node_ptr curr = reactor->head;

		while (curr != NULL)
		{
			size++;
			curr = curr->next;
		}

		struct pollfd fds[size];
		curr = reactor->head;

		memset(fds, 0, sizeof(fds));

		while (curr != NULL)
		{
			fds[i].fd = curr->fd;
			fds[i].events = POLLIN;
			fds[i].revents = 0;

			curr = curr->next;
			i++;
		}

		int ret = poll(fds, i, -1);

		if (ret < 0)
		{
			perror("\033[0;31m[ERROR]\033[0;37m poll() failed");
			return NULL;
		}

		for (i = 0; i < size; ++i)
		{
			if (fds[i].revents & POLLIN)
			{
				reactor_node_ptr curr = reactor->head;

				for (unsigned int j = 0; j < i; ++j)
					curr = curr->next;

				void *handler_ret = curr->handler(fds[i].fd, reactor);

				if (handler_ret == NULL && fds[i].fd != reactor->head->fd)
				{
					reactor_node_ptr curr_node = reactor->head;
					reactor_node_ptr prev_node = NULL;

					while (curr_node != NULL && curr_node->fd != fds[i].fd)
					{
						prev_node = curr_node;
						curr_node = curr_node->next;
					}

					prev_node->next = curr_node->next;

					free(curr_node);
				}

				continue;
			}

			else if ((fds[i].revents & POLLHUP || fds[i].revents & POLLNVAL || fds[i].revents & POLLERR) && fds[i].fd != reactor->head->fd)
			{
				reactor_node_ptr curr_node = reactor->head;
				reactor_node_ptr prev_node = NULL;

				while (curr_node != NULL && curr_node->fd != fds[i].fd)
				{
					prev_node = curr_node;
					curr_node = curr_node->next;
				}

				prev_node->next = curr_node->next;

				free(curr_node);
			}
		}
	}

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Reactor thread finished.\n");

	return reactor;
}

void *createReactor() {
	reactor_t_ptr react = NULL;

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Creating reactor...\n");

	if ((react = (reactor_t_ptr)malloc(sizeof(reactor_t))) == NULL)
	{
		perror("\033[0;31m[ERROR]\033[0;37m malloc() failed");
		return NULL;
	}

	react->thread = 0;
	react->head = NULL;
	react->running = false;

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Reactor created.\n");

	return react;
}

void startReactor(void *react) {
	if (react == NULL)
	{
		errno = EINVAL;
		perror("\033[0;31m[ERROR]\033[0;37m startReactor() failed");
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	if (reactor->running || reactor->head == NULL)
		return;

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Starting reactor thread...\n");

	reactor->running = true;

	pthread_create(&reactor->thread, NULL, reactorRun, reactor);

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Reactor thread started.\n");
}

void stopReactor(void *react) {
	if (react == NULL)
	{
		errno = EINVAL;
		perror("\033[0;31m[ERROR]\033[0;37m stopReactor() failed");
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	if (!reactor->running)
		return;

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Stopping reactor thread gracefully...\n");

	reactor->running = false;

	pthread_cancel(reactor->thread);

	// In case the thread is blocked on poll()
	// Ensure that the thread is cancelled by joining it, and then detach it
	// Prevents memory leaks
	pthread_join(reactor->thread, NULL);
	pthread_detach(reactor->thread);

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Reactor thread stopped and detached.\n");
}

void addFd(void *react, int fd, handler_t handler) {
	if (react == NULL)
	{
		errno = EINVAL;
		perror("\033[0;31m[ERROR]\033[0;37m addFd() failed");
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;
	reactor_node_ptr node = (reactor_node_ptr)malloc(sizeof(reactor_node));

	if (node == NULL)
	{
		perror("\033[0;31m[ERROR]\033[0;37m malloc() failed");
		return;
	}

	node->fd = fd;
	node->handler = handler;
	node->next = NULL;

	if (reactor->head == NULL)
		reactor->head = node;

	else
	{
		reactor_node_ptr curr = reactor->head;

		while (curr->next != NULL)
			curr = curr->next;

		curr->next = node;
	}
}

void WaitFor(void *react) {
	if (react == NULL)
	{
		errno = EINVAL;
		perror("\033[0;31m[ERROR]\033[0;37m WaitFor() failed");
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	if (!reactor->running)
		return;

	fprintf(stdout, "\033[0;35m[INFO]\033[0;37m Reactor thread joined.\n");
	pthread_join(reactor->thread, NULL);
}