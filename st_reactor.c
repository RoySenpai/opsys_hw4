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
		fprintf(stderr, "%s reactorRun() failed: %s\n", C_PREFIX_ERROR, strerror(EINVAL));
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
			fprintf(stderr, "%s poll() failed: %s\n", C_PREFIX_ERROR, strerror(errno));
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

	fprintf(stdout, "%s Reactor thread finished.\n", C_PREFIX_INFO);

	return reactor;
}

void *createReactor() {
	reactor_t_ptr react = NULL;

	fprintf(stdout, "%s Creating reactor...\n", C_PREFIX_INFO);

	if ((react = (reactor_t_ptr)malloc(sizeof(reactor_t))) == NULL)
	{
		fprintf(stderr, "%s malloc() failed: %s\n", C_PREFIX_ERROR, strerror(errno));
		return NULL;
	}

	react->thread = 0;
	react->head = NULL;
	react->running = false;

	fprintf(stdout, "%s Reactor created.\n", C_PREFIX_INFO);

	return react;
}

void startReactor(void *react) {
	if (react == NULL)
	{
		fprintf(stderr, "%s startReactor() failed: %s\n", C_PREFIX_ERROR, strerror(EINVAL));
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	if (reactor->head == NULL)
	{
		fprintf(stderr, "%s Tried to start a reactor without registered file descriptors.\n", C_PREFIX_WARNING);
		return;
	}

	else if (reactor->running)
	{
		fprintf(stderr, "%s Tried to start a reactor that's already running.\n", C_PREFIX_WARNING);
		return;
	}

	fprintf(stdout, "%s Starting reactor thread...\n", C_PREFIX_INFO);

	reactor->running = true;

	pthread_create(&reactor->thread, NULL, reactorRun, reactor);

	fprintf(stdout, "%s Reactor thread started.\n", C_PREFIX_INFO);
}

void stopReactor(void *react) {
	if (react == NULL)
	{
		fprintf(stderr, "%s stopReactor() failed: %s\n", C_PREFIX_ERROR, strerror(EINVAL));
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;

	if (!reactor->running)
	{
		fprintf(stderr, "%s Tried to stop a reactor that's not currently running.\n", C_PREFIX_WARNING);
		return;
	}

	fprintf(stdout, "%s Stopping reactor thread gracefully...\n", C_PREFIX_INFO);

	reactor->running = false;

	/*
	 * In case the thread is blocked on poll(), we ensure that the thread
	 * is cancelled by joining and detaching it.
	 * This prevents memory leaks.
	*/
	if (pthread_cancel(reactor->thread) != 0)
	{
		fprintf(stderr, "%s pthread_cancel() failed: %s\n", C_PREFIX_ERROR, strerror(errno));
		return;
	}

	if (pthread_join(reactor->thread, NULL) != 0)
	{
		fprintf(stderr, "%s pthread_join() failed: %s\n", C_PREFIX_ERROR, strerror(errno));
		return;
	}

	pthread_detach(reactor->thread);

	// Reset reactor pthread.
	reactor->thread = 0;

	fprintf(stdout, "%s Reactor thread stopped and detached.\n", C_PREFIX_INFO);
}

void addFd(void *react, int fd, handler_t handler) {
	if (react == NULL)
	{
		fprintf(stderr, "%s addFd() failed: %s\n", C_PREFIX_ERROR, strerror(EINVAL));
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;
	reactor_node_ptr node = (reactor_node_ptr)malloc(sizeof(reactor_node));

	if (node == NULL)
	{
		fprintf(stderr, "%s malloc() failed: %s\n", C_PREFIX_ERROR, strerror(errno));
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
		fprintf(stderr, "%s WaitFor() failed: %s\n", C_PREFIX_ERROR, strerror(EINVAL));
		return;
	}

	reactor_t_ptr reactor = (reactor_t_ptr)react;
	void *ret = NULL;

	if (!reactor->running)
		return;

	fprintf(stdout, "%s Reactor thread joined.\n", C_PREFIX_INFO);
	
	if (pthread_join(reactor->thread, ret) != 0)
		fprintf(stderr, "%s pthread_join() failed: %s\n", C_PREFIX_ERROR, strerror(errno));

	if (ret == NULL)
		fprintf(stderr, "%s Reactor thread fatal error: %s", C_PREFIX_ERROR, strerror(errno));
}