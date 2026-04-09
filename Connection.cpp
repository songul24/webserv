#include "Connection.hpp"


Connection::Connection(Server* srv, int fd): _fd(fd), _server(srv)
, _parsed(false), _sentLen(0), _respLen(0), _response(""), _last_active(time(NULL)){}

Connection::Connection() : _fd(-1), _server(NULL), _parsed(false),
               _sentLen(0), _respLen(0), _last_active(0) {}

// Connection::Connection(Connection const &other){}
Connection::~Connection()
{
        if(_fd != -1)
        {
                close(_fd);
                _fd = -1;
        }
}
// Connection& Connection::operator=(Connection const &other){}


int     Connection::getFd() {return _fd;}
Server* Connection::getServer() {return _server;}
bool    Connection::getParsed() {return _parsed;}
int     Connection::getSentlen() {return _sentLen;}
int     Connection::getRespLen() {return _respLen;}
std::string&    Connection::getResponse() {return _response;}
time_t          Connection::get_Lastactive() {return _last_active;}


void    Connection::setSentlen(int sentLen) {_sentLen = sentLen;}
void    Connection::setParsed(bool paresd) {_parsed = paresd;}
void    Connection::setResponse(const std::string& response) {_response = response;}
void    Connection::setRespLen(int respLen) {_respLen = respLen;}
void    Connection::setLastactive(time_t last_active) {_last_active = last_active;}