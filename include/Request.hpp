#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <vector>
#include <algorithm>

class	Request
{
	private:
		std::string							method;
		std::string							path;
		std::string							version;
		std::map<std::string, std::string>	heads;

	public:
		Request( void );
		Request( const Request &old );
		Request	&operator=( const Request &old );

		void	setMethod( std::string m );
		void	setPath( std::string p );
		void	setVersion( std::string v );
		void	setHeaders( std::map<std::string, std::string> h );

		std::string							getMethod( void ) const;
		std::string							getPath( void ) const;
		std::string							getVersion( void ) const;
		std::map<std::string, std::string>	getHeaders( void ) const;

		void	print_heads( void );
		~Request( void );

};
