#include "../include/Connection.hpp"
#include "../include/Server.hpp"
#include "../include/Request_header.hpp"
#include "../include/Request_line.hpp"
#include "../include/configfile.hpp"



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
        if(_fd != -1)
        {
                close(_fd);
                _fd = -1;
        }
}

void Connection::parseRequest(const char *buf)
{
	_raw_request += buf;

	size_t header_end = _raw_request.find("\r\n\r\n");

	if (header_end == std::string::npos)
		return;

	if (!_header_parsed)
	{
		parse_request(_raw_request, _request);

		if (_request.getStop())
			return;

		parse_headers(_raw_request, _request);

		_header_parsed = true;
	}

	header_end += 4;

	std::map<std::string, std::string>      headers = _request.getHeaders();
	std::string                             content_length_str;

	if (headers.find("content-length") != headers.end())
		content_length_str = headers["content-length"];

	if (content_length_str.empty())
        {
        	if (_request.getMethod() == "POST")
        	{
                        std::cout << "411 Length Required" << std::endl;
        		_request.setStop(true);
        		return;
        	}

        	_parsed = true;
        	return;
        }

	size_t body_length = std::atoi(content_length_str.c_str());

	if (body_length > _request.getMaxBodySize())
	{
		std::cout << "413 Content Too Large\n";
		_request.setStop(true);
		return;
	}

	size_t received_body = _raw_request.size() - header_end;

	if (received_body < body_length)
		return;

	_request.setBody(_raw_request.substr(header_end, body_length));

	_is_there_body = true;
	_parsed = true;
}


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



void            Connection::setSentlen(size_t sentLen) {_sentLen = sentLen;}
void            Connection::setParsed(bool paresd) {_parsed = paresd;}
void            Connection::setResponse(const std::string& response) {_response = response;}
void            Connection::setRespLen(size_t respLen) {_respLen = respLen;}
void            Connection::setLastactive(time_t last_active) {_last_active = last_active;}
void            Connection::setIsThereBody( bool t_or_f ) {_is_there_body = t_or_f;}
void            Connection::setHeaderParsed( bool t_or_f ) {_header_parsed = t_or_f;}
void            Connection::setRawRequest( std::string raw ) {_raw_request = raw;}

