#include "../include/WebServer.hpp"


std::map<std::string, std::string>      setCgiEnv(Connection client)
{
        std::map<std::string, std::string> env; 
        Request req = client.getRequest();
       
        env["REQUEST_METHOD"] = req.getMethod();
        env["SERVER_PROTOCOL"] = req.getVersion();  

        std::string fullPath = req.getPath();
        size_t queryPos = fullPath.find('?');
        if (queryPos != std::string::npos)
        {
            env["PATH_INFO"] = fullPath.substr(0, queryPos);
            env["QUERY_STRING"] = fullPath.substr(queryPos + 1);
        } 
        else
        {
            env["PATH_INFO"] = fullPath;
            env["QUERY_STRING"] = "";
        }       
        
        std::map<std::string, std::string> headers = req.getHeaders();

        if (headers.count("Content-Length"))
            env["CONTENT_LENGTH"] = headers["Content-Length"];

        if (headers.count("Content-Type"))
            env["CONTENT_TYPE"] = headers["Content-Type"];      
        if (headers.count("Cookie"))
            env["HTTP_COOKIE"] = headers["Cookie"];     
       
        
        if (!client.getServer()->getConfig().server_names.empty())
            env["SERVER_NAME"] = client.getServer()->getConfig().server_names[0];
        else
            env["SERVER_NAME"] = "localhost";   
        

        env["SERVER_PORT"] = client.getServer()->getPort().c_str();  

        env["GATEWAY_INTERFACE"] = "CGI/1.1";
        env["SERVER_SOFTWARE"] = "Webserv/1.0";
        env["PATH_TRANSLATED"] = client.getServer()->getConfig().root + env["PATH_INFO"]; 
        env["SCRIPT_ROOT"] = client.getServer()->getConfig().root;
        
        return env;
}

void    print_errno(const std::string& str, bool flag)
{
        int err = errno;
        if (flag && err != 0)
            std::cerr << str << ": " << strerror(err) << std::endl;
        else
            std::cerr << "Error: " << str << std::endl;
}
int    set_nonblock(int fd)
{
        //first for saving old flags
        int flag = fcntl(fd, F_GETFL, 0);
        if(flag == -1)
                return (print_errno("fcntl F_GETFL", true), 1);
        //we add nonblock flag to old flags
        if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
                return (print_errno("fcntl F_SETFL", true), 1);
        return 0;
}

int    add_to_epoll(int epfd, int fd, uint32_t events)
{
        epoll_event ev;
        ev.events  = events;
        ev.data.fd = fd;
        if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
                return (print_errno("epoll_ctl ADD", true), 1);
        return 0;
}

int    mod_to_epoll(int epfd, int fd, uint32_t events)
{
        epoll_event ev;
        ev.events  = events;
        ev.data.fd = fd;
        if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
                return (print_errno("epoll_ctl MOD", true), 1);
        return 0;
}

char    **map_to_env(const std::map<std::string, std::string>& env)
{
        char **result = new char*[env.size() + 1];
        int i = 0;
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
        {
                std::string entry = it->first + "=" + it->second;
                result[i++] = strdup(entry.c_str());
        }
        result[i] = NULL;
        return result;
}

void    free_env(char **env)
{
        for (int i = 0; env[i]; i++)
                free(env[i]);
        delete[] env;
}