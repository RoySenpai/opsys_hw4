#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "reactor.h"

void *reactorRun(void *react) {
    if (react == NULL)
    {
        perror("react");
        return NULL;
    }

    reactor_t *reactor = (reactor_t *)react;
    reactor_node *curr = reactor->head;

    while (true)
    {
        struct pollfd fds[reactor->size];

        for (int i = 0; i < reactor->size; ++i)
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

        for (int i = 0; i < reactor->size; ++i)
        {
            if (fds[i].revents & POLLIN)
            {
                reactor_node *curr = reactor->head;

                for (int j = 0; j < i; ++j)
                    curr = curr->next;

                curr->handler(fds[i].fd, curr->handler_arg);
            }

            else if (fds[i].revents & POLLHUP)
            {
                reactor_node *curr = reactor->head;

                while (curr->next != NULL && curr->next->fd != fds[i].fd)
                    curr = curr->next;

                close(fds[i].fd);

                free(curr->next);

                curr->next = curr->next->next;

                --reactor->size;
            }
        }
    }

    return reactor;
}

void *createReactor() {
    reactor_t *react = (reactor_t *)malloc(sizeof(reactor_t));
    if (react == NULL) {
        perror("malloc");
        return NULL;
    }
    react->head = NULL;
    react->size = 0;
    react->running = false;
    return react;
}

void startReactor(void *react) {
    if (react == NULL)
    {
        perror("react");
        return;
    }
    reactor_t *reactor = (reactor_t *)react;

    if (reactor->running)
        return;

    reactor->running = true;
    pthread_create(&reactor->thread, NULL, reactorRun, reactor);
}

void stopReactor(void *react) {
    if (react == NULL)
    {
        perror("react");
        return;
    }
    reactor_t *reactor = (reactor_t *)react;

    if (!reactor->running)
        return;

    reactor->running = false;
    pthread_cancel(reactor->thread);
}

void addFd(void *react, int fd, handler_t handler) {
    if (react == NULL)
    {
        perror("react");
        return;
    }

    reactor_t *reactor = (reactor_t *)react;
    reactor_node *node = (reactor_node *)malloc(sizeof(reactor_node));

    if (node == NULL)
    {
        perror("malloc");
        return;
    }

    node->fd = fd;
    node->handler = handler;
    node->next = NULL;

    if (reactor->head == NULL)
        reactor->head = node;
    
    else 
    {
        reactor_node *curr = reactor->head;

        while (curr->next != NULL)
            curr = curr->next;

        curr->next = node;
    }

    ++reactor->size;
}

void WaitFor(void *react) {
    if (react == NULL)
    {
        perror("react");
        return;
    }

    reactor_t *reactor = (reactor_t *)react;

    if (!reactor->running)
        return;

    pthread_join(reactor->thread, NULL);
}