#ifndef _REACTOR_H
#define _REACTOR_H

#include <stdbool.h>
#include <pthread.h>

#define PORT 8080

#define LISTENQ 4096

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
	 * @brief The handler function argument.
	 * @note The first node is always the listening socket, 
	 * 			and its handler argument is always the reactor list.
	*/
	void *handler_arg;

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
	 * @brief The size of the linked list.
	 * @note The first node is always the listening socket,
	 * 			and thus it should always start with 1.
	*/
	unsigned int size;

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

#endif