#include "../include/Server.hpp"


Server::Server(ServerConfig config)
: _config(config), _fd(-1), _port("")
{
        std::cout << "ip -----" << _config.ip << std::endl;
        std::ostringstream oss;
        oss << _config.port;
        _port = oss.str();
}

Server::Server(Server const &other)
: _config(other._config), _fd(other._fd), _port(other._port){}

Server& Server::operator=(const Server& other)
{
        if(this != &other)
        {
                _config = other._config;
                _fd = other._fd;
                _port = other._port;
        }
        return *this;
}

Server::~Server()
{
        if(_fd != -1)
                close(_fd);
}


int                     Server::getFd() const{return (_fd);}
const std::string&      Server::getPort() const {return _port;}
const ServerConfig&     Server::getConfig() const {return (_config);}

void                    Server::setFd(int fd){_fd = fd;}
void                    Server::setConfig(const ServerConfig& config){_config = config;}




int    Server::setup()
{
        addrinfo setup;
        addrinfo *res = NULL;
        addrinfo *p;

        memset(&setup, 0, sizeof(setup));
        setup.ai_family = AF_INET;            //ipv4
        setup.ai_socktype = SOCK_STREAM;        //TCP
        setup.ai_flags = AI_PASSIVE;            //use my ip
        setup.ai_protocol = IPPROTO_TCP;

        int status = getaddrinfo(_config.ip.c_str(), _port.c_str(), &setup, &res);
        if(status != 0)
            return (print_errno(gai_strerror(status), false), 1);

        for(p = res; p != NULL; p = p->ai_next)
        {
                if((_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
                {
                        print_errno("Socket failed", true);
                        continue;
                }
                if(set_nonblock(_fd))
                {
                        close(_fd);
                        _fd = -1;
                        continue;
                }
                int yes = 1;

                //If this port is in the TIME_WAIT state, let me grab it anyway.(when ctrl c)
                if(setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
                {
                        close(_fd);
                        _fd = -1;
                        print_errno("setsockopt failed", true);
                        continue;
                }
                if (bind(_fd, p->ai_addr, p->ai_addrlen) == -1)
                {
                        close(_fd);
                        _fd = -1;
                        print_errno("bind failed", true);
		        continue;
		}
	        break;
        }

        freeaddrinfo(res); 
	if (p == NULL || _fd == -1)
                throw std::runtime_error("Server failed to bind to in port:" + _port);
        if (listen(_fd, BACKLOG) == -1)
        {
                print_errno("listen failed", true);
                close(_fd);
		return (1);
	}
        std::cout << "Server listening on server ip" << _config.ip.c_str() << "in Port: " << _port << std::endl;
        return 0;
}
