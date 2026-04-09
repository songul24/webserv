#pragma once


#include <algorithm>
#include <iostream>
#include <string>
#include <vector>


#include "Connection.hpp"

class Server 
{
        private:
            int                         _fd;
            std::string                 _port;
            std::string                 _ip;
        

        public:
                Server(const std::string& port, const std::string& ip, int fd);
                // Server(Server const &other);
	        // Server& operator=(const Server& other);
		~Server();

                int     getFd();
                std::string getPort();
                std::string getIp();
                int setup();  // socket(), bind(), listen(), epoll_create()
                

};

int    set_nonblock(int fd);
void    print_errno(const std::string& str, bool flag);