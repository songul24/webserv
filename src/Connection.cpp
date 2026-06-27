#include "../include/Connection.hpp"
#include "../include/Server.hpp"
#include "../include/Configfile.hpp"
#include "../include/Request.hpp"


Connection::Connection(Server* srv, int fd, int epollfd): _epollfd(epollfd), _fd(fd), _server(srv)
, _parsed(false), _sentLen(0),_response("") , _respLen(0), _last_active(time(NULL))
, _is_there_body(false), _header_parsed(false), _raw_request(""), _pipeFd(-1), _pid(-1), _cgiTime(0), _cgiOutput("") {}


Connection::Connection() : _epollfd(-1), _fd(-1), _server(NULL), _parsed(false),
               _sentLen(0), _respLen(0), _last_active(0), _is_there_body(false),
               _raw_request(""), _pipeFd(-1), _pid(-1), _cgiTime(0), _cgiOutput("") {}

Connection::Connection(Connection const &other): _epollfd(other._epollfd), _fd(other._fd), _server(other._server)
, _parsed(other._parsed), _sentLen(other._sentLen),_response(other._response) , _respLen(other._respLen), _last_active(other._last_active), _request(other._request), _is_there_body(other._is_there_body), _raw_request(other._raw_request), _pipeFd(other._pipeFd), _pid(other._pid), _cgiTime(other._cgiTime), _cgiOutput(other._cgiOutput){}

Connection& Connection::operator=(Connection const &other)
{
        if(this != &other)
        {
                _epollfd = other._epollfd;
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
                _pipeFd = other._pipeFd;
                _pid = other._pid;
                _cgiTime = other._cgiTime;
                _cgiOutput = other._cgiOutput;
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




int             Connection::getFd() const {return _fd;}
int             Connection::getEpollFd() const {return _epollfd;}
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
const Request&  Connection::getRequestRef(void) const {return _request;}
int             Connection::getpipeFd() const {return _pipeFd;}
pid_t             Connection::getpid() const {return _pid;}
time_t             Connection::getcgiTime() const {return _cgiTime;}
std::string     Connection::getCgioutput() const {return _cgiOutput;}


void            Connection::setSentlen(size_t sentLen) {_sentLen = sentLen;}
void            Connection::setParsed(bool paresd) {_parsed = paresd;}
void            Connection::setResponse(const std::string& response) {_response = response;}
void            Connection::setRespLen(size_t respLen) {_respLen = respLen;}
void            Connection::setLastactive(time_t last_active) {_last_active = last_active;}
void            Connection::setIsThereBody( bool t_or_f ) {_is_there_body = t_or_f;}
void            Connection::setHeaderParsed( bool t_or_f ) {_header_parsed = t_or_f;}
void            Connection::setRawRequest( std::string raw ) {_raw_request = raw;}
void            Connection::setpipeFd(int pipeFd) {_pipeFd = pipeFd;}
void            Connection::setpid(pid_t pid) {_pid = pid;}
void            Connection::setcgiTime(time_t cgiTime) {_cgiTime = cgiTime;}
void            Connection::setCgioutput(const std::string& cgiOutput) {_cgiOutput = cgiOutput;}



void Connection::parseRequest(const std::string& data)  // ← Changé de const char* à const std::string&
{
    _raw_request += data;
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