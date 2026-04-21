#pragma once

// #include <algorithm>
// #include <iostream>
// #include <string>
// #include <vector>

#include "configfile.hpp"
#include "Connection.hpp"



struct s_cookie {
        std::string name;
        std::string id;
        std::map<std::string, std::string> attributes;
        time_t  last_active;
    
        s_cookie(std::string n, std::string v) : name(n), id(v) {}
};

class Server 
{
        private:
                ServerConfig                    _config;
                int                             _fd;
                std::string                     _port;
                std::map<std::string, s_cookie> _cookies;
        
                
        public:
                Server(ServerConfig config);
                Server(Server const &other);
                Server& operator=(const Server& other);
		~Server();

                int     getFd() const;
                const std::string& getPort() const;
                const ServerConfig& getConfig() const;

                void    setFd(int fd);
                void    setConfig(const ServerConfig& config);
                int setup();  // socket(), bind(), listen(), epoll_create()
                
                std::string     parseCookies(const std::map<std::string, std::string>& header);
                std::string     gen_cookie();
                std::string     mod_cookie(const std::string& att);
};

int    set_nonblock(int fd);
void    print_errno(const std::string& str, bool flag);