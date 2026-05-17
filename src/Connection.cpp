#include "../include/Connection.hpp"
#include "../include/Server.hpp"
#include "../include/configfile.hpp"
#include "../include/Request.hpp"


Connection::Connection(Server* srv, int fd): _fd(fd), _server(srv)
, _parsed(false), _sentLen(0),_response("") , _respLen(0), _last_active(time(NULL))
, _is_there_body(false), _header_parsed(false), _raw_request("") {}


Connection::Connection() : _fd(-1), _server(NULL), _parsed(false),
               _sentLen(0), _respLen(0), _last_active(0), _is_there_body(false),
               _raw_request("") {}

Connection::Connection(Connection const &other): _fd(other._fd), _server(other._server)
, _parsed(other._parsed), _sentLen(other._sentLen),_response(other._response) , _respLen(other._respLen), _last_active(other._last_active), _request(other._request), _is_there_body(other._is_there_body), _raw_request(other._raw_request){}

Connection& Connection::operator=(Connection const &other)
{
        if(this != &other)
        {
                _fd = other._fd;
                _server = other._server;
                _parsed = other._parsed;
                _sentLen = other._sentLen;
                _response = other._response;
                _respLen = other._respLen;
                _last_active = other._last_active;
                _request = other._request;
                _is_there_body = other._is_there_body;
                _raw_request = other._raw_request;
        }
        return (*this);
}

Connection::~Connection()
{
        // if(_fd != -1)
        // {
        //         std::cout << "client " << _fd << "closed!" << std::endl;
        //         // close(_fd);
        //         // _fd = -1;
        // }
}

// void Connection::parseRequest(const std::string& data)  // ← Changé de const char* à const std::string&
// {
//         //  _raw_request += std::string(buf);  //malika
//     _raw_request += data;  // ← Plus de problème de \0
//     if(_parsed)
//         return;
    
//     size_t header_end = _raw_request.find("\r\n\r\n");
//     if (header_end == std::string::npos)
//         return;
 
//     _request.setMaxBodySize(_server->getConfig().max_body_size);    
//     parse_request(_raw_request, _request);
    
// //     _raw_request.erase(0, header_end + 4); 
//     if (_request.isError())
//     {
//         std::cerr << "Parse error: " << _request.getError() << std::endl;
//         _parsed = true;
//         return;
//     }
 
//     if (_request.isComplete())
//     {
//         if (!_request.getBody().empty())
//             _is_there_body = true;
//         _parsed = true;
//     }
// }
void Connection::parseRequest(const std::string& data)  // ← Changé de const char* à const std::string&
{
        //  _raw_request += std::string(buf);  //malika
    _raw_request += data;  // ← Plus de problème de \0
    if(_parsed)
        return;
    
    // size_t header_end = _raw_request.find("\r\n\r\n");
    // if (header_end == std::string::npos)
    //     return;
 
    // _request.setMaxBodySize(_server->getConfig().max_body_size);    

    if (_request.getStatus() != Request::BODY)
    {
        size_t header_end = _raw_request.find("\r\n\r\n");
        if (header_end == std::string::npos)
            return;
        _request.setMaxBodySize(_server->getConfig().max_body_size);
    }

    parse_request(_raw_request, _request);
    
    // _raw_request.erase(0, header_end + 4); 
    if (_request.isError())
    {
        std::cerr << "Parse error: " << _request.getError() << std::endl;
        _parsed = true;
        return;
    }
 
    if (_request.isComplete())
    {
        if (!_request.getBody().empty())
            _is_there_body = true;
        _parsed = true;
    }
}
// void Connection::parseRequest(const char *buf, int bytes)
// {
//     _raw_request.append(buf, bytes);

//     if (_raw_request.find("\r\n\r\n") == std::string::npos)
//         return;

//     if (!_header_parsed)
//     {
//         parse_request(_raw_request, _request);
//         if (_request.getStop())
//             return;
//         parse_headers(_raw_request, _request);
//         _header_parsed = true;
//     }

//     std::string content_length = _request.getHeaders()["Content-Length"];
//     if (!content_length.empty())
//     {
//         size_t body_len = (size_t)std::atoi(content_length.c_str());
//         size_t header_end = _raw_request.find("\r\n\r\n") + 4;
//         size_t content_received = _raw_request.size() - header_end;

//         if (_request.getMaxBodySize() != 0 && body_len > _request.getMaxBodySize())
//         {
//             _request.setStop(true);
//             _request.setErrorCode(413);
//             return;
//         }
//         if (content_received < body_len)
//             return;

//         _request.setBody(_raw_request.substr(header_end, body_len));
//     }

//     _is_there_body = true;
//     _parsed = true;
// }

int             Connection::getFd() const {return _fd;}
Server*         Connection::getServer() const {return _server;}
bool            Connection::getParsed() const {return _parsed;}
int             Connection::getSentlen() const {return _sentLen;}
int             Connection::getRespLen() const {return _respLen;}
std::string     Connection::getResponse() const {return _response;}
time_t          Connection::get_Lastactive() const {return _last_active;}
bool            Connection::getIsThereBody( void ) const {return _is_there_body;}
bool            Connection::getHeaderParsed( void ) const {return _header_parsed;}
std::string     Connection::getRawRequest( void ) const {return _raw_request;}
Request         Connection::getRequest(void) const  {return _request;}
// const std::string& Connection::getRawRequestRef( void ) const {return _raw_request;}
const Request&  Connection::getRequestRef(void) const {return _request;}


void            Connection::setSentlen(size_t sentLen) {_sentLen = sentLen;}
void            Connection::setParsed(bool paresd) {_parsed = paresd;}
void            Connection::setResponse(const std::string& response) {_response = response;}
void            Connection::setRespLen(size_t respLen) {_respLen = respLen;}
void            Connection::setLastactive(time_t last_active) {_last_active = last_active;}
void            Connection::setIsThereBody( bool t_or_f ) {_is_there_body = t_or_f;}
void            Connection::setHeaderParsed( bool t_or_f ) {_header_parsed = t_or_f;}
void            Connection::setRawRequest( std::string raw ) {_raw_request = raw;}

// // The parse request method :)
// void    Connection::parseRequest( const char *buf )
// {
//         _raw_request += buf;

//         if (_raw_request.find("\r\n\r\n") == std::string::npos)
//                 return ;

//         if (!_header_parsed)
//         {
//                 parse_request(_raw_request, _request);
//                 parse_headers(_raw_request, _request);
//                 _header_parsed = true;
//         }

//         std::string     content_length = _request.getHeaders()["Content-Length"];
//         if (!content_length.empty())
//         {
//                 size_t  body_len = std::atoi(content_length.c_str());
//                 size_t  header_end = _raw_request.find("\r\n\r\n") + 4;
//                 size_t  content_received = _raw_request.size() - header_end;

//                 if (body_len > _request.getMaxBodySize())
//                 {
//                         std::cout << "413 Content Too Large" << std::endl;
// 		        _request.setStop(true);
//                         return ;
//                 }

//                 if (body_len > content_received)
//                         return ;

//                 _request.setBody(_raw_request.substr(header_end, body_len));
//                 _is_there_body = true;
//         }
        
//         _is_there_body = true;
//         _parsed = true;
// }