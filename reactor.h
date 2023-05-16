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

#ifndef _REACTOR_H
#define _REACTOR_H

/*
** Include this file before including system headers.  By default, with
** C99 support from the compiler, it requests POSIX 2001 support.  With
** C89 support only, it requests POSIX 1997 support.  Override the
** default behaviour by setting either _XOPEN_SOURCE or _POSIX_C_SOURCE.
*/

/* _XOPEN_SOURCE 700 is loosely equivalent to _POSIX_C_SOURCE 200809L */
/* _XOPEN_SOURCE 600 is loosely equivalent to _POSIX_C_SOURCE 200112L */
/* _XOPEN_SOURCE 500 is loosely equivalent to _POSIX_C_SOURCE 199506L */

#if !defined(_XOPEN_SOURCE) && !defined(_POSIX_C_SOURCE)
	#if __STDC_VERSION__ >= 199901L
		#define _XOPEN_SOURCE 600   /* SUS v3, POSIX 1003.1 2004 (POSIX 2001 + Corrigenda) */
	#else
		#define _XOPEN_SOURCE 500   /* SUS v2, POSIX 1003.1 1997 */
	#endif /* __STDC_VERSION__ */
#endif /* !_XOPEN_SOURCE && !_POSIX_C_SOURCE */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

/*
 * @brief The port on which the server listens.
 * @note The default port is 9034.
*/
#define SERVER_PORT 9034

/*
 * @brief The maximum number of clients that can connect to the server.
 * @note The default number is 8192 clients.
*/
#define MAX_QUEUE 8192

/*
 * @brief The maximum number of bytes that can be read from a socket.
 * @note The default number is 1024 bytes.
*/
#define MAX_BUFFER 1024

/*
 * @brief A handler function for a file descriptor.
 * @param fd The file descriptor.
 * @param arg A pointer to an argument that the handler may use.
 * @return A pointer to something that the handler may return.
*/
typedef void *(*handler_t)(int fd, void *arg);

/*
 * @brief A node in the reactor linked list.
 */
typedef struct _reactor_node
{
	/*
	 * @brief The file descriptor.
	 * @note The first node is always the listening socket.
	*/
	int fd;

	/*
	 * @brief The handler function to call when the file descriptor is ready.
	 * @note The first node is always the listening socket, and its handler is always
	 * 			to accept a new connection and add it to the reactor.
	*/
	handler_t handler;

	/*
	 * @brief The next node in the linked list.
	 * @note For the last node, this is NULL.
	*/
	struct _reactor_node *next;
} reactor_node, *reactor_node_ptr;

/*
 * @brief A reactor object - a linked list of file descriptors and their handlers.
 */
typedef struct _reactor_t
{
	/*
	 * @brief The thread in which the reactor is running.
	 * @note The thread is created in startReactor() and deleted in stopReactor().
	 * 			The thread function is reactorRun().
	*/
	pthread_t thread;

	/*
	 * @brief The first node in the linked list.
	 * @note The first node is always the listening socket.
	*/
	reactor_node *head;

	/*
	 * @brief A boolean value indicating whether the reactor is running.
	 * @note The value is set to true in startReactor() and to false in stopReactor().
	*/
	bool running;
} reactor_t, *reactor_t_ptr;

/*
 * @brief Create a reactor object - a linked list of file descriptors and their handlers.
 * @return A pointer to the created object, or NULL if failed.
 * @note The returned pointer must be freed.
 */
void *createReactor();

/*
 * @brief Start executing the reactor, in a new thread.
 * @param react A pointer to the reactor object.
 * @return void
 */
void startReactor(void *react);

/*
 * @brief Stop the reactor - delete the thread.
 * @param react A pointer to the reactor object.
 * @return void
 */
void stopReactor(void *react);

/*
 * @brief Add a file descriptor to the reactor.
 * @param react A pointer to the reactor object.
 * @param fd The file descriptor to add.
 * @param handler The handler function to call when the file descriptor is ready.
 * @return void
 */
void addFd(void *react, int fd, handler_t handler);

/*
 * @brief Wait for the reactor to finish.
 * @param react A pointer to the reactor object.
 * @return void
 */
void WaitFor(void *react);


/*
 * @brief A signal handler for SIGINT.
 * @note This function is called when the user presses CTRL+C.
 * 			It stops the reactor, closes all sockets and frees all memory,
 * 			Then, it exits the program.
 * @note This function is registered to SIGINT in main().
*/
void signal_handler();

/*
 * @brief A handler for a client socket.
 * @param fd The client socket file descriptor.
 * @param arg The reactor.
 * @return The reactor on success, NULL otherwise.
 * @note This function is called when a client sends a message to the server,
 * 			and prints the message.
*/
void *client_handler(int fd, void *arg);

/*
 * @brief A handler for the server socket.
 * @param fd The server socket file descriptor.
 * @param arg The reactor.
 * @return The reactor on success, NULL otherwise.
 * @note This function is called when a new client connects to the server,
 * 			and adds the client to the reactor.
*/
void *server_handler(int fd, void *arg);

#endif