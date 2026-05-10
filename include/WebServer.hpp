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



int             set_nonblock(int fd);
int             add_to_epoll(int epfd, int fd, uint32_t events);
int             mod_to_epoll(int epfd, int fd, uint32_t events);
bool            exists(const std::string &path);
bool            is_method_allowed(const std::string& method, const std::vector<std::string>& methods);

std::map<std::string, std::string>      setCgiEnv(Connection& client);
const LocationConfig*                   find_location(const ServerConfig& config, const std::string& url);

char                    **map_to_env(const std::map<std::string, std::string>& env);
std::string                    Delete_method(Connection& client);
void                    free_env(char **env);
std::string     Get_method(Connection &client);
std::string     run_cgi(const std::string& cgiPath, std::string scriptPath, Connection& client, const std::string& bodyPath);

std::string     generateRandom_name();
std::string     handle_dir_cgi(const ServerConfig& srv, const LocationConfig* lc,std::string& path, std::string& script);
std::string     is_cgi(const ServerConfig& srv, const LocationConfig* loc, const std::string& path);
std::string buildResponse(int code, const std::string &body, const std::string &type, const ServerConfig* conf);
std::string errorResponse(int code, const std::string& type, const ServerConfig* conf);
std::string    Post_method(Connection &cnx);
std::string     read_File(const std::string& path);