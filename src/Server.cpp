#include "../include/Server.hpp"

Server::Server(const std::string& port, const std::string& ip, int fd): _port(port)
, _fd(fd), _ip(ip){}

// Server::Server(Server const &other): _port(other._port), _epoll_fd(other._epoll_fd)
// , _fd(other._fd), _clients(other._clients){}

// Server& Server::operator=(const Server& other)
// {
//         if(this != &other)
//         {
//                 _port = other._port;
//                 _epoll_fd = other._epoll_fd;
//                 _fd = other._fd;
//                 _clients = other._clients;
//         }
//         return *this;
// }

Server::~Server()
{
        if(_fd != -1)
                close(_fd);
}

int     Server::getFd(){return (_fd);}
std::string Server::getPort(){return (_port);}
std::string Server::getIp(){return (_ip);}


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

int    Server::setup()
{
        addrinfo setup = addrinfo();
        addrinfo *res = NULL;
        addrinfo *p;

        memset(&setup, 0, sizeof(setup));
        setup.ai_family = AF_INET;            //ipv4
        setup.ai_socktype = SOCK_STREAM;        //TCP
        setup.ai_flags = AI_PASSIVE;            //use my ip
        setup.ai_protocol = IPPROTO_TCP;

        int status = getaddrinfo(_ip.c_str(), _port.c_str(), &setup, &res);
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
        freeaddrinfo(res); // all done with this structure
	if (p == NULL || _fd == -1)
                throw std::runtime_error("Server failed to bind to " + _ip + ":" + _port);
        if (listen(_fd, BACKLOG) == -1)
        {
                print_errno("listen failed", true);
                close(_fd);
		return (1);
	}
        return 0;
}

