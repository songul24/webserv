#pragma once


#include <algorithm>
#include <iostream>
#include <string>
#include <sys/types.h>    // For portability
#include <sys/socket.h>   // For socket-related constants
#include <netdb.h>        // For getaddrinfo, struct addrinfo, etc.
#include <arpa/inet.h>
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
#include <sstream>


#include "Request.hpp"
class Server;

#define BACKLOG 1024   // how many pending connections queue will hold
#define MAX_EVENTS  1024



class Connection {
        private:
                int                     _epollfd;
                int                     _fd;
                Server*                 _server;
                bool                    _parsed;

                size_t                  _sentLen;
                std::string             _response;
                size_t                  _respLen;
                time_t                  _last_active;

                
                Request         _request;
                bool            _is_there_body;
                bool            _header_parsed;
                std::string     _raw_request;

                int             _pipeFd;
                pid_t             _pid;
                time_t          _cgiTime;
                std::string     _cgiOutput;
                
        public:
                Connection(Server* srv, int fd, int epollfd);
                Connection();
                Connection(Connection const &other);
		Connection & operator=(Connection const &other);
		~Connection();
                

                void            parseRequest(const std::string& data);

                //getters
                int             getEpollFd() const;
                int             getFd() const;
                int             getSentlen() const;
                int             getRespLen() const;
                std::string     getResponse() const;
                Server*         getServer() const;
                bool            getParsed() const;
                time_t          get_Lastactive() const;
                
                bool            getIsThereBody( void ) const;
                bool            getHeaderParsed( void ) const;
                std::string     getRawRequest( void ) const;
                Request         getRequest(void) const;
                const Request&  getRequestRef(void) const;
                int             getpipeFd() const;
                pid_t           getpid() const;
                time_t         getcgiTime() const;
                std::string    getCgioutput() const;

                //setters
                void    setSentlen(size_t sentLen);
                void    setParsed(bool paresd);
                void    setResponse(const std::string& response);
                void    setRespLen(size_t respLen);
                void    setLastactive(time_t last_active);
                
                void     setIsThereBody( bool t_or_f );
                void     setHeaderParsed( bool t_or_f );
                void     setRawRequest( std::string raw );
                void     setpipeFd(int pipeFd);
                void     setpid(pid_t pid);
                void     setcgiTime(time_t cgiTime);
                void    setCgioutput(const std::string& cgiOutput);

};



