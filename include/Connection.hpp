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
#include <stdexcept>
#include <csignal>
class Server;

#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_EVENTS  64
volatile sig_atomic_t g_run = 1;



class Connection {
        private:
                int             _fd;
                Server*          _server;
                bool            _parsed;
                int             _sentLen;
                std::string    _response;
                int             _respLen;
                time_t          _last_active;


        public:
                Connection(Server* srv, int fd);
                Connection();
		// Connection(Connection const &other);
		~Connection();
		// Connection & operator=(Connection const &other);
                
                // The parse request method :)
                void    parseRequest( const char *buf );

                //getters
                int             getFd();
                int             getSentlen();
                int             getRespLen();
                std::string&    getResponse();
                Server*         getServer();
                bool            getParsed();
                time_t          get_Lastactive();

                //setters
                void    setSentlen(int sentLen);
                void    setParsed(bool paresd);
                void    setResponse(const std::string& response);
                void    setRespLen(int respLen);
                void    setLastactive(time_t last_active);
};