#include "../include/WebServer.hpp"
// #include "Cgi.cpp"


volatile sig_atomic_t g_run = 1;

WebServer::WebServer(): _epoll_fd(-2){}

WebServer::WebServer(WebServer const &other) :_epoll_fd(other._epoll_fd), _servers(other._servers), _fd_to_server(other._fd_to_server), _clients(other._clients){}

WebServer& WebServer::operator=(const WebServer& other)
{
        if(this != &other)
        {
                _epoll_fd = other._epoll_fd;
                _servers = other._servers;
                _clients = other._clients;
                _fd_to_server = other._fd_to_server;
        }
        return *this;
}
WebServer::~WebServer()
{
        if(_epoll_fd != -2)
                close(_epoll_fd);
}


void    handle_signal(int sig)
{
        (void)sig;
        g_run = 0;
}

void    setup_signals(void)
{
        signal(SIGPIPE, SIG_IGN);  // ignore broken pipe client close connection in response 
        signal(SIGQUIT, SIG_IGN);  // Ctrl /
        signal(SIGINT,  handle_signal); // Ctrl+C
        signal(SIGTERM, handle_signal); //kill
}

void    WebServer::setupServer(const std::string& configPath)
{
        setup_signals();

        Configfile config;
        std::string content = config.readFile(configPath);
        std::vector<std::string> tokens = config.tokenize(content);
        std::vector<ServerConfig> config_servers = config.parseServers(tokens);

        _fd_to_server.clear();
        _servers.reserve(config_servers.size());
        for(size_t i = 0; i < config_servers.size(); i++)
                _servers.push_back(Server(config_servers[i]));
        for(size_t i = 0; i < config_servers.size(); i++)
        {
                if(_servers[i].setup() != 0)
                        throw std::runtime_error("Failed to setup server in port:" + _servers[i].getPort());
                _fd_to_server[_servers[i].getFd()] = &_servers[i];
        }
}




void    WebServer::handle_new_connection(Server *srv)
{
        while(true) 
        {
                int client_fd = accept(srv->getFd(), NULL, NULL);
                if (client_fd < 0) 
                        break;
                if(set_nonblock(client_fd))
                {
                        close(client_fd);
                        continue;
                }
                if(add_to_epoll(_epoll_fd, client_fd, EPOLLIN))
                {
                        close(client_fd);
                        continue;
                }   
                _clients[client_fd] = Connection(srv, client_fd);
                std::cout << "New client fd=" << client_fd << std::endl;
        }
}

void    WebServer::close_connection(int fd)
{
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        _clients.erase(fd);
        close(fd);
}

void    WebServer::handle_client_response(int fd)
{
        std::cout << "-------Send client resp----> " << fd << std::endl;

        int sent_byte = _clients[fd].getSentlen();
        int total_len =  _clients[fd].getRespLen();
        const std::string& resp = _clients[fd].getResponse();
        int new_sent_byte = sent_byte;
        
        int n = send(fd, resp.c_str() + sent_byte, total_len - sent_byte, MSG_NOSIGNAL);
        if(n <= 0)
        {
                close_connection(fd);
                return ;
        }
        new_sent_byte = sent_byte + n;
        _clients[fd].setSentlen(new_sent_byte);
        if(new_sent_byte < total_len)
        {
                mod_to_epoll(_epoll_fd, fd, EPOLLOUT);
        }
        else
        {
                std::cout<<"******************RESPONSE SENT TO CLIENT:"<< fd <<"*******************"<<std::endl;
                close_connection(fd);
        }
}

std::string     buildRedirect(int code, const std::string& new_url)
{
        std::ostringstream oss;
        std::string statusText;

        switch(code)
        {
                case 301: statusText = "Moved Permanently"; break;
                case 302: statusText = "Found"; break;
                case 303: statusText = "See Other"; break;
                case 307: statusText = "Temporary Redirect"; break;
                case 308: statusText = "Permanent Redirect"; break;
                default:  statusText = "Moved"; break;
        }

        oss << "HTTP/1.1 " << code << " " << statusText << "\r\nConnection: close\r\n";
        oss << "Location: " << new_url << "\r\n";
        oss << "Content-Length: 0\r\n";
        oss << "\r\n";

        return oss.str();
}

const LocationConfig* setup_methods(Connection& client, std::string* resp)
{
        const ServerConfig& conf = client.getServer()->getConfig();
        Request req = client.getRequest();
        std::string method = req.getMethod();
        
        const LocationConfig* loc = find_location(conf, req.getPath());
        const std::vector<std::string>& allowed = (loc && !loc->methods.empty()) 
        ? loc->methods : conf.methods; 
        if(!loc)
        {
                if(!req.getBody().empty())
                        std::remove(req.getBody().c_str());
                *resp = errorResponse(404, "text/html", &conf);    
        }
        else if (!loc->redirect.empty())
        {
                if(!req.getBody().empty())
                        std::remove(req.getBody().c_str());
                *resp = buildRedirect(loc->redirect_code, loc->redirect);    
        }
        else if (!is_method_allowed(method, allowed))
        {
                if(!req.getBody().empty())
                        std::remove(req.getBody().c_str());
                *resp = errorResponse(405, "text/html", &conf);
        }
        return loc;
}

void    WebServer::execute_methods(int fd)
{
        
        std::string method = _clients[fd].getRequest().getMethod();
        std::string response;


        const LocationConfig *loc = setup_methods(_clients[fd], &response);
        if(response.empty() && loc)
        {
                const ServerConfig& conf = _clients[fd].getServer()->getConfig();
                std::string root = (loc && !loc->root.empty()) ? loc->root : conf.root;
                std::string uri  = _clients[fd].getRequest().getPath();
                if(loc)
                        uri = uri.substr(loc->path.size());
                if (!root.empty() && root[root.size() - 1] == '/' && !uri.empty() && uri[0] == '/')
                        uri = uri.substr(1);
                else if (!root.empty() && root[root.size() - 1] != '/' && !uri.empty() && uri[0] != '/')
                        root += '/';
                std::string file_path = root + uri;

                if(method == "DELETE")
                        response = Delete_method(_clients[fd], file_path);
                else if(method == "GET")
                        response = Get_method(_clients[fd], loc, file_path);
                else if(method == "POST")
                        response = Post_method(_clients[fd], loc, file_path);
                else
                        response = errorResponse(404, "text/html", &_clients[fd].getServer()->getConfig());
        }

        _clients[fd].setResponse(response);
        _clients[fd].setRespLen(response.size());
        _clients[fd].setSentlen(0);
}



// void    WebServer::handle_client_request(int fd)
// {
//         char buf[10000];
//         int bytes = recv(fd, buf, sizeof(buf) - 1, 0);
//         if(bytes <= 0)
//         {
//                 close_connection(fd);
//                 return ;
//         }
//         buf[bytes] = '\0';
//         _clients[fd].parseRequest(buf);
//         if(_clients[fd].getRequest().isError())
//         {
//                 int code = _clients[fd].getRequest().getError();
//                 ServerConfig conf = _clients[fd].getServer()->getConfig();
//                 std::string resp = errorResponse(code, "text/html", &conf);
//                 send(fd, resp.c_str(), resp.size(), MSG_NOSIGNAL);
//                 close_connection(fd);
//         }
//         else if(_clients[fd].getParsed())
//         {
//                 execute_methods(fd);
//                 handle_client_response(fd);
//         }
// }

void    WebServer::handle_client_request(int fd)
{
        char buf[10000];
        int bytes = recv(fd, buf, sizeof(buf) - 1, 0);
        if(bytes <= 0)
        {
                close_connection(fd);
                return ;
        }
        buf[bytes] = '\0';
        _clients[fd].parseRequest(buf);
        if(_clients[fd].getRequest().isError())
        {
                int code = _clients[fd].getRequest().getError();
                ServerConfig conf = _clients[fd].getServer()->getConfig();
                std::string resp = errorResponse(code, "text/html", &conf);
                _clients[fd].setResponse(resp);
                _clients[fd].setRespLen(resp.size());
                _clients[fd].setSentlen(0);
                handle_client_response(fd);
        }
        else if(_clients[fd].getParsed())
        {
                execute_methods(fd);
                handle_client_response(fd);
        }
}
void    WebServer::check_timeout()
{
        std::map<int, Connection>::iterator it;
        it = _clients.begin();
        while (it != _clients.end()) 
        {
                int fd = it->first;
                Connection &client = it->second;
                if (difftime(time(NULL), client.get_Lastactive()) > 60)
                {
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        _clients.erase(it++); 
                        std::cout << "timeout fd=" << fd << std::endl;
                }
                else
                        ++it;
        }
}

void    WebServer::runServer()
{
        // Create the epoll instance
        _epoll_fd = epoll_create(1);
        if (_epoll_fd == -1)
                throw std::runtime_error("epoll_create failed: " + std::string(strerror(errno)));

        size_t j = 0;
        for(size_t i = 0; i < _servers.size(); i++)
        {
                if(add_to_epoll(_epoll_fd, _servers[i].getFd(), EPOLLIN))
                        j++;
        }
        if(j == _servers.size())
                throw std::runtime_error("All Server Initializations Failed Program Cannot Proceed");

        epoll_event events[MAX_EVENTS];
        std::cout << "servers: waiting for connections...\n" << std::endl;

        while(g_run)
        {
                std::cout << "-----Epoll Loop-----------" << std::endl;
                int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, 5000);

                if (n_ready < 0)
                {
                        if (errno == EINTR)
                                continue; 
                        throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));
                }

                //loop over all events
                for (int i = 0; i < n_ready; i++) 
                {
                        int fd      = events[i].data.fd;
                        uint32_t ev = events[i].events;
                        Server *srv = NULL;
                        if (_fd_to_server.count(fd))
                                srv = _fd_to_server[fd];

                        // ── new connection ──
                        if (srv) 
                        {
                                std::cout << "-----------New Connection----- " << n_ready << std::endl;
                                handle_new_connection(srv);
                                continue;
                        }

                        // ── error or hangup ──
                        if (ev & (EPOLLERR | EPOLLHUP)) 
                        {
                                std::cout << "Error/hangup on fd="<< fd << std::endl;
                                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                                close_connection(fd);
                                continue;
                        }

                        // ── data ready to recv ──
                        if (ev & EPOLLIN)
                        {
                                std::cout << "-----New request-----------" << std::endl;
                                handle_client_request(fd);
                                if(_clients.count(fd))
                                        _clients[fd].setLastactive(time(NULL));
                        }

                        // ── ready to send ──
                        if (ev & EPOLLOUT)
                        {
                                std::cout << "-------Finish sending response---------" << std::endl;
                                handle_client_response(fd);
                                if(_clients.count(fd))
                                        _clients[fd].setLastactive(time(NULL));
                        }
                }
                check_timeout();
        }
        std::cout << "-----End Of loop-----------" << std::endl;
}