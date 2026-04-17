// #include "../include/WebServer.hpp"

// WebServer::WebServer(): _epoll_fd(-2){}

// // WebServer::WebServer(WebServer const &other){}

// // WebServer& WebServer::operator=(const WebServer& other){}

// WebServer::~WebServer()
// {
//         // if(_listen_fd != -1)
//         //         close(_listen_fd);
//         if(_epoll_fd != -2)
//                 close(_epoll_fd);
// }


// void    handle_signal(int sig)
// {
//         (void)sig;
//         g_run = 0;
// }

// void    setup_signals(void)
// {
//         signal(SIGPIPE, SIG_IGN);  // ignore broken pipe client close connection in response 
//         signal(SIGQUIT, SIG_IGN);  // Ctrl /
//         signal(SIGINT,  handle_signal); // Ctrl+C
//         signal(SIGTERM, handle_signal); //kill
// }

// void    WebServer::setupServer()
// {
//         setup_signals();
//         memset(&_fd_to_server, 0, sizeof(_fd_to_server));
//         // _fd_to_server[fd] = &servers[i];
// }


// int    add_to_epoll(int epfd, int fd, uint32_t events)
// {
//         epoll_event ev;
//         ev.events  = events;
//         ev.data.fd = fd;
//         if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
//                 return (print_errno("epoll_ctl ADD", true), 1);
//         return 0;
// }

// int    mod_to_epoll(int epfd, int fd, uint32_t events)
// {
//         epoll_event ev;
//         ev.events  = events;
//         ev.data.fd = fd;
//         if(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
//                 return (print_errno("epoll_ctl MOD", true), 1);
//         return 0;
// }

// void    WebServer::handle_new_connection(Server *srv)
// {
//         while(true) 
//         {
//                 int client_fd = accept(srv->getFd(), NULL, NULL);
//                 if (client_fd < 0) 
//                         break;
//                 if(set_nonblock(client_fd))
//                 {
//                         close(client_fd);
//                         continue;
//                 }
//                 if(add_to_epoll(_epoll_fd, client_fd, EPOLLIN))
//                 {
//                         close(client_fd);
//                         continue;
//                 }       
//                 _clients[client_fd] = Connection(srv, client_fd);
//                 std::cout << "New client fd= " << client_fd << std::endl;
//         }
// }

// void    WebServer::close_connection(int fd)
// {
//         epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
//         _clients.erase(fd);
// }

// void    WebServer::handle_client_response(int fd)
// {
//         int sent_byte = _clients[fd].getSentlen();
//         int total_len =  _clients[fd].getRespLen();
//         const std::string& resp = _clients[fd].getResponse();
//         int new_sent_byte = sent_byte;
//         int n = send(fd, resp.c_str() + sent_byte, total_len - sent_byte, MSG_NOSIGNAL);
//         if(n <= 0)
//         {
//                 close_connection(fd);
//                 return ;
//         }
//         new_sent_byte = sent_byte + n;
//         _clients[fd].setSentlen(new_sent_byte);
//         if(new_sent_byte < total_len)
//         {
//                 mod_to_epoll(_epoll_fd, fd, EPOLLOUT);
//         }
//         else
//         {
//                 std::cout<<"******************RESPONSE SENT TO CLIENT:"<< fd <<"*******************"<<std::endl;
//                 close_connection(fd);
//         }
// }

// void    WebServer::handle_client_request(int fd)
// {
//         char buf[10000] = {0};
//         int bytes = recv(fd, buf, sizeof(buf), 0);
//         if(bytes <= 0)
//                 close_connection(fd);
//         else
//         {
//                 // _clients[fd].parse_request(buf);
//                 if(_clients[fd].getParsed())
//                 {
//                         // execute_request(fd);
//                         handle_client_response(fd);
//                 }
//         }
// }

// void    WebServer::check_timeout()
// {
//         std::map<int, Connection>::iterator it;
//         it = _clients.begin();
//         while (it != _clients.end()) 
//         {
//                 int fd = it->first;
//                 Connection &client = it->second;
//                 if (difftime(time(NULL), client.get_Lastactive()) > 60)
//                 {
//                         it = _clients.erase(it++);
//                         std::cout << "timeout fd=" << fd << std::endl;
//                 }
//                 else
//                         ++it;
//         }
// }

// void    WebServer::runServer()
// {
//         // Create the epoll instance
//         _epoll_fd = epoll_create(1);

//         int j = 0;
//         for(size_t i = 0; i < _servers.size(); i++)
//         {
//                 if(add_to_epoll(_epoll_fd, _servers[i].getFd(), EPOLLIN))
//                         j++;
//         }
//         if(j == _servers.size())
//                 throw std::runtime_error("All Server Initializations Failed Program Cannot Proceed");

//         epoll_event events[MAX_EVENTS];
//         std::cout << "servers: waiting for connections...\n" << std::endl;

//         while(g_run)
//         {
//                 int n_ready = epoll_wait(_epoll_fd, events, MAX_EVENTS, 5000);

//                 if (n_ready < 0)
//                         throw std::runtime_error("epoll_wait: " + std::string(strerror(errno)));

//                 //loop over all events
//                 for (int i = 0; i < n_ready; i++) 
//                 {
//                         int fd      = events[i].data.fd;
//                          uint32_t ev = events[i].events;
//                         Server *srv = _fd_to_server[fd];
//                         // ── new connection ──
//                         if (srv) 
//                         {
//                                 handle_new_connection(srv);
//                                 continue;
//                         }

//                         // ── error or hangup ──
//                         if (ev & (EPOLLERR | EPOLLHUP)) 
//                         {
//                                 std::cout << "Error/hangup on fd="<< fd << std::endl;
//                                 // remove_from_epoll(_epoll_fd, fd);
//                                 close_connection(fd);
//                                 continue;
//                         }

//                         // ── data ready to recv ──
//                         if (ev & EPOLLIN)
//                         {
//                                 handle_client_request(fd);
//                                 if(_clients.count(fd))
//                                         _clients[fd].setLastactive(time(NULL));
//                         }

//                         // ── ready to send ──
//                         if (ev & EPOLLOUT)
//                         {
//                                 handle_client_response(fd);
//                                 if(_clients.count(fd))
//                                         _clients[fd].setLastactive(time(NULL));
//                         }
//                 }
//                 check_timeout();
//         }
// }