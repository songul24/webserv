#pragma once


#include <algorithm>
#include <iostream>
#include <string>
#include <sys/types.h>    // For portability
#include <sys/socket.h>   // For socket-related constants
#include <netdb.h>        // For getaddrinfo, struct addrinfo, etc.
// #include <netinet/in.h>
#include <arpa/inet.h>
// #include <sys/wait.h>
#include <cerrno>   // For errno
#include <cstring>  // For strerror
#include <unistd.h>
#include <fcntl.h> // for fcnlt()
#include <sys/epoll.h>
#include <stdint.h> //for uint32_t
#include <ctime>

#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_EVENTS  64