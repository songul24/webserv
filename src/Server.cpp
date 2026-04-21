#include "../include/Server.hpp"

Server::Server(ServerConfig config)
: _config(config), _fd(-1)
{
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
                _port = other._port;
        }
        return *this;
}

Server::~Server()
{
        if(_fd != -1)
                close(_fd);
}

int     Server::getFd() const{return (_fd);}
const std::string& Server::getPort() const {return _port;}
const ServerConfig& Server::getConfig() const {return (_config);}

void    Server::setFd(int fd){_fd = fd;}
void    Server::setConfig(const ServerConfig& config){_config = config;}


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
        freeaddrinfo(res); // all done with this structure
	if (p == NULL || _fd == -1)
                throw std::runtime_error("Server failed to bind to in port:" + _port);
        if (listen(_fd, BACKLOG) == -1)
        {
                print_errno("listen failed", true);
                close(_fd);
		return (1);
	}
        return 0;
}

std::string     get_rand()
{
        long long random_num = (long long)std::rand() + (long long)time(NULL);

        std::stringstream ss;
        ss << std::hex << random_num;
        std::string id = ss.str();

        return id;
}

std::string     Server::gen_cookie()
{
        std::string id;
        id = get_rand();
        std::map<std::string, s_cookie>::iterator it = _cookies.find(id);
        while(it != _cookies.end())
                id = get_rand();

        s_cookie newCookie("session_id", id);
        newCookie.attributes["Max-Age"] = "3600";
        newCookie.attributes["Path"] = "/";
        newCookie.attributes["HttpOnly"] = "";
        newCookie.last_active = time(NULL);
        _cookies[id] = newCookie;
        std::string response = "Set-Cookie: session_id=" + id + "; HttpOnly; Max-Age=3600; Path=/\r\n";
        return response;
}

std::string     Server::mod_cookie(const std::string& att)
{
        size_t pos = att.find("session_id=");
        std::string value;
        if (pos != std::string::npos)
        {
                value = att.substr(pos + 11); // skip  "session_id="
                size_t end = value.find(";");
                if (end != std::string::npos)
                    value = value.substr(0, end);
        }
        else
                return (gen_cookie());
        
        std::map<std::string, s_cookie>::iterator it = _cookies.find(value);
        if(it != _cookies.end())
        {
                if(time(NULL) - it->second.last_active >= 3600)
                {
                        _cookies.erase(value);
                        return ("Set-Cookie: session_id=" + value + "; Max-Age=0\r\n" + gen_cookie());
                }
                it->second.last_active = time(NULL);
        }
        else
                return (gen_cookie());
        return ("");
}

std::string     Server::parseCookies(const std::map<std::string, std::string>& header)
{
        // std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        std::map<std::string, std::string>::const_iterator it = header.find("cookie");

        if (it != header.end())
                return (mod_cookie(it->second));
        else 
                return (gen_cookie());
}