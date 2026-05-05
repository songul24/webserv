#pragma once

#include "Server.hpp"
#include "Connection.hpp"
#include "configfile.hpp"
#include "Request.hpp"

// #define MAX_CLIENT 1024
#define MAX_FD 1024


class WebServer 
{
        private:
                int                         _epoll_fd;
                std::vector<Server>         _servers;
                std::map<int, Server*>      _fd_to_server;
                std::map<int, Connection>   _clients;

                WebServer(WebServer const &other);
                WebServer& operator=(const WebServer& other);

        public:
                WebServer();
                ~WebServer();

                void setupServer(const std::string& configPath);
                void runServer();

                void handle_new_connection(Server *srv);
                void handle_client_request(int fd);
                void close_connection(int fd);
                void handle_client_response(int fd);
                void check_timeout();
                void execute_methods(int fd);
   

};



int set_nonblock(int fd);
int add_to_epoll(int epfd, int fd, uint32_t events);
int mod_to_epoll(int epfd, int fd, uint32_t events);
void Deleth_method(Connection& client);
std::map<std::string, std::string>      setCgiEnv(Connection client);

char    **map_to_env(const std::map<std::string, std::string>& env);
void    free_env(char **env);
void Get_method(Connection &client, const std::map<std::string, std::string>& env);

