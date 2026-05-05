#include "Request.hpp"

Request::Request( void ) : method(""), path(""), version(""), query(""), heads(), max_body_size(0), stop(false)
{}

Request::Request( const Request &old ) : method(old.method), path(old.path), version(old.version), 
										query(old.query), heads(old.heads), max_body_size(old.max_body_size), stop(old.stop)
{}

Request	&Request::operator=( const Request &old )
{
	if (this != &old)
	{
		method = old.method;
		path = old.path;
		version = old.version;
		heads = old.heads;
		max_body_size = old.max_body_size;
		stop = old.stop;
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

void	Request::setQuery( std::string q )
{
	query = q;
}

void	Request::setHeaders( std::map<std::string, std::string> h )
{
	heads = h;
}

void	Request::setMaxBodySize( size_t m_b_s )
{
	max_body_size = m_b_s;
}

void	Request::setStop( bool s )
{
	stop = s;
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

std::string	Request::getQuery( void ) const
{
	return (query);
}

std::map<std::string, std::string>	Request::getHeaders( void ) const
{
	return (heads);
}

size_t								Request::getMaxBodySize( void ) const
{
	return (max_body_size);
}

bool								Request::getStop( void ) const
{
	return (stop);
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
