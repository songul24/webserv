#pragma once

#include "Server.hpp"
#include "Connection.hpp"
#include "configfile.hpp"
#include <map>

// #define MAX_CLIENT 1024
#define MAX_FD 1024


class WebServer 
{
        private:
                int                         _epoll_fd;
                std::vector<Server>         _servers;
                Server*                     _fd_to_server[MAX_FD];
                std::map<int, Connection>   _clients;
                // Configuration
                Configfile                    _config;
                std::vector<ServerConfig>     _server_configs;

        public:
                WebServer();
                // WebServer(WebServer const &other);
            // WebServer& operator=(const WebServer& other);
        ~WebServer();

                void setupServer();
                void runServer();

                void handle_new_connection(Server *srv);
                void handle_client_request(int fd);
                void close_connection(int fd);
                void handle_client_response(int fd);
                void check_timeout();

};