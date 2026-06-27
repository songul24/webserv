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

                std::cout << "############ method -> " << method << " path### " << _clients[fd].getRequest().getPath() << std::endl; 

        const LocationConfig *loc = setup_methods(_clients[fd], &response);
        if(response.empty() && loc)
        {
                const ServerConfig& conf = _clients[fd].getServer()->getConfig();
                std::string root = (loc && !loc->root.empty()) ? loc->root : conf.root;
                std::string uri  = _clients[fd].getRequest().getPath();
                if(uri.size() >= loc->path.size())
                        uri = uri.substr(loc->path.size());
                else
                        uri = ""; 
                if (!root.empty() && root[root.size() - 1] == '/' && !uri.empty() && uri[0] == '/')
                        uri = uri.substr(1);
                else if (!root.empty() && root[root.size() - 1] != '/' && !uri.empty() && uri[0] != '/')
                        root += '/';
                std::string file_path = root + uri;

std::cout << "############ path -> " << file_path << std::endl; 
                if(method == "DELETE")
                        response = Delete_method(_clients[fd], file_path, root);
                else if(method == "GET")
                        response = Get_method(_clients[fd], loc, file_path);
                else if(method == "POST")
                        response = Post_method(_clients[fd], loc, file_path);
                else
                        response = errorResponse(405, "text/html", &_clients[fd].getServer()->getConfig());
        }

        if(response == "CGI")
        {
                int pipefd = _clients[fd].getpipeFd();
                _cgi_to_client[pipefd] = fd;
                return;
        }
        _clients[fd].setResponse(response);
        _clients[fd].setRespLen(response.size());
        _clients[fd].setSentlen(0);
        handle_client_response(fd);
}

void    WebServer::handle_client_request(int fd)
{
        
        char buf[8192] = {0};
        bool is_cgi = false;
        int bytes;
        int clientFD;
        if(_cgi_to_client.count(fd))
        {
                is_cgi = true;
                clientFD = _cgi_to_client[fd];
        }
        if(is_cgi)
                bytes = read(fd, buf, sizeof(buf));
        else
                bytes = recv(fd, buf, sizeof(buf), 0);
        if(!bytes)
        {
                if(is_cgi)
                {
                        int status;
                        waitpid(_clients[clientFD].getpid(), &status, WNOHANG);
                        ServerConfig conff = _clients[clientFD].getServer()->getConfig();
                        std::string output = _clients[clientFD].getCgioutput();
                        std::string response = cgi_response(output, &conff);
                        _clients[clientFD].setResponse(response);
                        _clients[clientFD].setRespLen(response.size());
                        _clients[clientFD].setSentlen(0);
                        _cgi_to_client.erase(fd);
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        _clients[clientFD].setpipeFd(-1);
                        _clients[clientFD].setpid(-1);
                        handle_client_response(clientFD);
                }
                else
                {
                        std::cout << "Client fd=" << _clients[fd].getFd() << " disconnected!" << std::endl;
                        close_connection(fd);
                }
                return;
        }
        else if(bytes < 0)
        {
                if(is_cgi)
                {
                        kill_proccess(_clients[clientFD].getpid());
                        _cgi_to_client.erase(fd);
                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        _clients[clientFD].setpipeFd(-1);
                        _clients[clientFD].setpid(-1);
                        ServerConfig conff = _clients[clientFD].getServer()->getConfig();
                        std::string response = errorResponse(502, "text/html", &conff);
                        _clients[clientFD].setResponse(response);
                        _clients[clientFD].setRespLen(response.size());
                        _clients[clientFD].setSentlen(0);
                        handle_client_response(clientFD);
                }
                return;
        }
        else
        {
                if(is_cgi)
                {
                        std::string cgi_output = _clients[clientFD].getCgioutput() + std::string(buf, bytes);
                        _clients[clientFD].setCgioutput(cgi_output);
                        return;
                }
                std::cout << "New request to fd=" << _clients[fd].getFd() << std::endl;
                _clients[fd].parseRequest(std::string(buf, bytes));
                if(_clients[fd].getRequest().isError())
                {
                        std::cout << "---------HERE IN ERRPRRRRRRRRRR" << std::endl;
                        if(!_clients[fd].getRequest().getBody().empty())
                                std::remove(_clients[fd].getRequest().getBody().c_str());
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
                        std::cout << "---------HERE IN EXECUTE METHODS" << std::endl;
                        execute_methods(fd);
                        
                }
        }
}

void WebServer::close_connection(int fd)
{
        if(!_clients.count(fd))
        {
                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                return;
        }
        int pipeFd = _clients[fd].getpipeFd();
        if(pipeFd != -1)
        {
                kill_proccess(_clients[fd].getpid());
                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, pipeFd, NULL);
                close(pipeFd);
                _cgi_to_client.erase(pipeFd);
                _clients[fd].setpipeFd(-1);
                _clients[fd].setpid(-1);
        }
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        _clients.erase(fd);
        close(fd);
}

void    WebServer::handle_client_response(int fd)
{
        if(!_clients.count(fd))
                return;
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
                _clients[client_fd] = Connection(srv, client_fd, _epoll_fd);
                std::cout << "New client fd=" << client_fd << " to server " <<  srv->getConfig().ip 
                << ":" << srv->getPort() << std::endl;
        }
}

void    WebServer::check_timeout()
{
        std::map<int, Connection>::iterator it;
        static time_t last_check = 0;
        time_t now = time(NULL);

        if (difftime(now, last_check) >= 2)
        {
                std::map<int, int>::iterator cgi_it = _cgi_to_client.begin();
                while (cgi_it != _cgi_to_client.end()) 
                {
                        // int fd = cgi_it->first;
                        int clientFD = cgi_it->second;
                        int pipeFd   = cgi_it->first;
                        if(!_clients.count(clientFD))
                        {
                                // orphaned pipe
                                std::map<int,int>::iterator to_del = cgi_it++;
                                _cgi_to_client.erase(to_del);
                                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, pipeFd, NULL);
                                close(pipeFd);
                                continue;
                        }

                        Connection& client = _clients[clientFD];
                        time_t cgi_time = client.getcgiTime(); 
                        if (difftime(now, cgi_time) > 7)
                        {
                                std::cout << "Timeout cgi of client fd=" << client.getFd() << std::endl;
                                std::map<int,int>::iterator to_del = cgi_it++;
                                _cgi_to_client.erase(to_del);
                                kill_proccess(client.getpid());
                                epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, pipeFd, NULL);
                                close(pipeFd);
                                client.setpipeFd(-1);
                                client.setpid(-1);

                                ServerConfig conff = client.getServer()->getConfig();
                                std::string response = errorResponse(504, "text/html", &conff);
                                client.setResponse(response);
                                client.setRespLen(response.size());
                                client.setSentlen(0);
                                handle_client_response(client.getFd());
                        }
                        else
                                ++cgi_it;
                }

                it = _clients.begin();
                while (it != _clients.end()) 
                {
                        int fd = it->first;
                        Connection &client = it->second;
                        Request::t_status status = client.getRequest().getStatus(); 

                        if ((difftime(now, client.get_Lastactive()) > 15 
                        && (status == Request::COMPLETE || status == Request::ERROR))
                        || (difftime(now, client.get_Lastactive()) > 13 
                        && (status != Request::COMPLETE && status != Request::ERROR)))
                        {
                                ServerConfig conf = client.getServer()->getConfig();
                                std::remove(client.getRequest().getBody().c_str());
                                std::string resp = errorResponse(408, "text/html", &conf);
                                std::cout << "Timeout client fd=" << client.getFd() << std::endl;
        
                                send(fd, resp.c_str(), resp.size(), MSG_NOSIGNAL);
                                it++;
                                close_connection(fd);
                        }
                        else
                                ++it;
                }
                last_check = now;
        }
}

void    WebServer::runServer()
{
        // Create the epoll instance
        _epoll_fd = epoll_create(1);
        if (_epoll_fd == -1)
                throw std::runtime_error("Epoll_create failed: " + std::string(strerror(errno)));

        size_t j = 0;
        for(size_t i = 0; i < _servers.size(); i++)
        {
                if(add_to_epoll(_epoll_fd, _servers[i].getFd(), EPOLLIN))
                        j++;
        }
        if(j == _servers.size())
                throw std::runtime_error("All Server Initializations Failed Program Cannot Proceed");

        epoll_event events[MAX_EVENTS];
        std::cout << "\nservers: waiting for connections...\n" << std::endl;

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
                                
                                if (_cgi_to_client.count(fd))
                                        handle_client_request(fd);
                                else if(_clients.count(fd))
                                {
                                        std::cout << "Error/hangup on fd="<< fd << std::endl;
                                        if (!_clients[fd].getRequest().getBody().empty())
                                                std::remove(_clients[fd].getRequest().getBody().c_str());
                                        close_connection(fd);
                                }
                                else
                                        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                                continue;
                        }

                        // ── data ready to recv ──
                        if (ev & EPOLLIN)
                        {
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