#include "Request.hpp"

Request::Request( void ) {}

Request::Request( const Request &old ) : method(old.method), path(old.path), version(old.version)
{}

Request	&Request::operator=( const Request &old )
{
	if (this != &old)
	{
		method = old.method;
		path = old.path;
		version = old.version;
	}
	return (*this);
}

// ---------------- SETTERS -----------------------------------

void	Request::setMethod( std::string m )
{
	method = m;
}

void	Request::setPath( std::string p )
{
	path = p;
}

void	Request::setVersion( std::string v )
{
	version = v;
}

void	Request::setHeaders( std::map<std::string, std::string> h )
{
	heads = h;
}

// ---------------- GETTERS -----------------------------------

std::string	Request::getMethod( void ) const
{
	return (method);
}

std::string	Request::getPath( void ) const
{
	return (path);
}

std::string	Request::getVersion( void ) const
{
	return (version);
}

std::map<std::string, std::string>	Request::getHeaders( void ) const
{
	return (heads);
}

// ---------------- PRINT -----------------------------------

void	Request::print_heads( void )
{
	std::map<std::string, std::string>::iterator	it = heads.begin();

	std::cout << "-------------------------------------------\n";
	while (it != heads.end())
	{
		std::string	key = it->first;
		std::string	value = it->second;
		std::cout << "[ " << key << " ] --> " << value << std::endl;
		it++;
	}
	std::cout << "-------------------------------------------\n";
}

// ---------------- DESTRUCTOR -----------------------------------

Request::~Request( void ) {}
