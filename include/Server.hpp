#pragma once


#include "Configfile.hpp"
#include "Connection.hpp"




class Server 
{
        private:
                ServerConfig                    _config;
                int                             _fd;
                std::string                     _port;
        
                
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
                int setup();  // socket(), bind(), listen()
                
};

int set_nonblock(int fd);
void    print_errno(const std::string& str, bool flag);