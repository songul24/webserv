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
#include <sys/stat.h>
#include <cstdio>
#include <dirent.h>

#include "Request.hpp"
class Server;

#define BACKLOG 10   // how many pending connections queue will hold
#define MAX_EVENTS  64




class Connection {
        private:
                int             _fd;
                Server*          _server;
                bool            _parsed;
                int             _sentLen;
                std::string    _response;
                int             _respLen;
                time_t          _last_active;
                // Need it in my request parsing 😝
                Request         _request;
                bool            _is_there_body;
                std::string     _raw_request;



        public:
                Connection(Server* srv, int fd);
                Connection();
		// Connection(Connection const &other);
		~Connection();
		// Connection & operator=(Connection const &other);
                
                // The parse request method :)
                void            parseRequest( const char *buf );

                //getters
                int             getFd() const;
                int             getSentlen() const;
                int             getRespLen() const;
                std::string    getResponse() const;
                Server*         getServer() const;
                bool            getParsed() const;
                time_t          get_Lastactive() const;
                // Need it in my request parsing 😝
                bool            getIsThereBody( void ) const;
                std::string     getRawRequest( void ) const;

                //setters
                void    setSentlen(int sentLen);
                void    setParsed(bool paresd);
                void    setResponse(const std::string& response);
                void    setRespLen(int respLen);
                void    setLastactive(time_t last_active);
                  // Need it in my request parsing 😝
                void     setIsThereBody( bool t_or_f );
                void     setRawRequest( std::string raw );
};