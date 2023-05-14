# Operation Systems (OSs) Course Assignment 4

### For Computer Science B.S.c Ariel University

**By Roy Simanovich and Linor Ronen**

## Description
In this assignment we created a Reactor (Reactor Design Pattern) that handles multiple client file descriptors (TCP socket)
and handles them accordingly, using an handler function for each file descriptor.

The Reactor library supports the following functions:
* `void *createReactor()` – Create a reactor object - a linked list of file descriptors and their handlers.
* `void startReactor(void *react)` – Start executing the reactor, in a new thread. 
* `void stopReactor(void *react)` – Stop the reactor - delete the thread.
* `void addFd(void *react, int fd, handler_t handler)` – Add a file descriptor to the reactor.
* `void WaitFor(void *react)` – Joins the reactor thread to the calling thread and wait for the reactor to finish.

## Requirements
* Linux machine (Ubuntu 22.04 LTS preferable)
* GNU C Compiler
* Make

## Building
```
# Cloning the repo to local machine.
git clone https://github.com/RoySenpai/opsys_hw4.git

# Building all the necessary files & the main programs.
make all

# Export shared libraries.
export LD_LIBRARY_PATH="."
```

## Running
```
# Run the reactor server
./react_server
```