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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "reactor.h"

void *reactorRun(void *react) {
    if (react == NULL)
    {
        fprintf(stderr, "reactorRun() failed: Reactor pointer is NULL\n");
        return NULL;
    }

    fprintf(stdout, "[INFO] Reactor thread started.\n");

    reactor_t_ptr reactor = (reactor_t_ptr)react;

    while (reactor->running)
    {
        struct pollfd fds[reactor->size];
        reactor_node_ptr curr = reactor->head;

        memset(fds, 0, sizeof(struct pollfd) * reactor->size);

        for (unsigned int i = 0; i < reactor->size; ++i)
        {
            fds[i].fd = curr->fd;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
            curr = curr->next;
        }

        int ret = poll(fds, reactor->size, -1);

        if (ret < 0)
        {
            perror("poll");
            return NULL;
        }

        for (unsigned int i = 0; i < reactor->size; ++i)
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

                    --reactor->size;
                }

                continue;
            }

            else if ((fds[i].revents & POLLHUP || fds[i].revents & POLLERR || fds[i].revents & POLLNVAL) && fds[i].fd != reactor->head->fd)
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

                --reactor->size;
            }
        }
    }

    fprintf(stdout, "[INFO] Reactor thread finished.\n");

    return reactor;
}

void *createReactor() {
    reactor_t_ptr react = NULL;

    fprintf(stdout, "[INFO] Creating reactor...\n");

    if ((react = (reactor_t_ptr)malloc(sizeof(reactor_t))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    react->head = NULL;
    react->size = 0;
    react->running = false;

    fprintf(stdout, "[INFO] Reactor created\n");

    return react;
}

void startReactor(void *react) {
    if (react == NULL)
    {
        fprintf(stderr, "startReactor() failed: Reactor pointer is NULL\n");
        return;
    }

    reactor_t_ptr reactor = (reactor_t_ptr)react;

    if (reactor->running || reactor->head == NULL || reactor->size == 0)
        return;

    fprintf(stdout, "[INFO] Starting reactor thread...\n");

    reactor->running = true;

    pthread_create(&reactor->thread, NULL, reactorRun, reactor);
}

void stopReactor(void *react) {
    if (react == NULL)
    {
        fprintf(stderr, "stopReactor() failed: Reactor pointer is NULL\n");
        return;
    }

    reactor_t_ptr reactor = (reactor_t_ptr)react;

    if (!reactor->running)
        return;

    fprintf(stdout, "[INFO] Stopping reactor thread...\n");

    reactor->running = false;

    pthread_cancel(reactor->thread);
}

void addFd(void *react, int fd, handler_t handler) {
    if (react == NULL)
    {
        fprintf(stderr, "addFd() failed: Reactor pointer is NULL\n");
        return;
    }

    reactor_t_ptr reactor = (reactor_t_ptr)react;
    reactor_node_ptr node = (reactor_node_ptr)malloc(sizeof(reactor_node));

    if (node == NULL)
    {
        perror("malloc() failed");
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

    reactor->size++;
}

void WaitFor(void *react) {
    if (react == NULL)
    {
        fprintf(stderr, "WaitFor() failed: Reactor pointer is NULL\n");
        return;
    }

    reactor_t_ptr reactor = (reactor_t_ptr)react;

    if (!reactor->running)
        return;

    fprintf(stdout, "[INFO] Reactor thread joined\n");
    pthread_join(reactor->thread, NULL);
}