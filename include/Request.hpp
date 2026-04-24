#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

class	Request
{
	private:
		std::string							method;
		std::string							path;
		std::string							version;
		std::map<std::string, std::string>	heads;
		std::string							body;

	public:
		Request( void );
		Request( const Request &old );
		Request	&operator=( const Request &old );

		void	setMethod( std::string m );
		void	setPath( std::string p );
		void	setVersion( std::string v );
		void	setHeaders( std::map<std::string, std::string> h );
		void	setBody( std::string b );

		std::string							getMethod( void ) const;
		std::string							getPath( void ) const;
		std::string							getVersion( void ) const;
		std::map<std::string, std::string>	getHeaders( void ) const;
		std::string							getBod( void ) const;

		void	print_heads( void );
		~Request( void );

};
