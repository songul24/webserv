#include "../include/Connection.hpp"
#include "../include/Server.hpp"
#include "../include/Request_header.hpp"
#include "../include/Request_line.hpp"

Connection::Connection(Server* srv, int fd): _fd(fd), _server(srv)
, _parsed(false), _sentLen(0),_response("") , _respLen(0), _last_active(time(NULL)){}

Connection::Connection() : _fd(-1), _server(NULL), _parsed(false),
               _sentLen(0), _respLen(0), _last_active(0), _is_there_body(false),
               _raw_request(NULL) {}

// // Connection::Connection(Connection const &other){}
Connection::~Connection()
{
        if(_fd != -1)
        {
                close(_fd);
                _fd = -1;
        }
}
// // Connection& Connection::operator=(Connection const &other){}



// The parse request method :)
void    Connection::parseRequest( const char *buf )
{
        _raw_request += buf;

        if (_raw_request.find("\r\n\r\n") == std::string::npos)
                return ;

        parse_request(_raw_request, _request);
        parse_headers(_raw_request, _request);

        std::string     content_length = _request.getHeaders()["Content-Length"];
        if (!content_length.empty())
        {
                size_t  body_len = std::atoi(content_length.c_str());
                size_t  header_end = _raw_request.find("\r\n\r\n") + 4;
                size_t  content_received = _raw_request.size() - header_end;

                if (body_len > content_received)
                        return ;

                _request.setBody(_raw_request.substr(header_end, body_len));
        }
        
        _is_there_body = true;
        _parsed = true;
}


int     Connection::getFd() const {return _fd;}
Server* Connection::getServer() const {return _server;}
bool    Connection::getParsed() const {return _parsed;}
int     Connection::getSentlen() const {return _sentLen;}
int     Connection::getRespLen() const {return _respLen;}
std::string    Connection::getResponse() const {return _response;}
time_t          Connection::get_Lastactive() const {return _last_active;}
bool            Connection::getIsThereBody( void ) const {return _is_there_body;}
std::string     Connection::getRawRequest( void ) const {return _raw_request;}


void    Connection::setSentlen(int sentLen) {_sentLen = sentLen;}
void    Connection::setParsed(bool paresd) {_parsed = paresd;}
void    Connection::setResponse(const std::string& response) {_response = response;}
void    Connection::setRespLen(int respLen) {_respLen = respLen;}
void    Connection::setLastactive(time_t last_active) {_last_active = last_active;}
void    Connection::setIsThereBody( bool t_or_f ) {_is_there_body = t_or_f;}
void    Connection::setRawRequest( std::string raw ) {_raw_request = raw;}


// void    Connection::executMethods()
// {
//         // if(_request.getMethod() == "POST")
//         //         // Post_method(*this);
//         // else if(_request.getMethod() == "GET")
//         //         Get_method(*this);
//         else if(_request.getMethod() == "DELETE")
//                 Delete_method(*this);
//         else
//                 return; 
// }