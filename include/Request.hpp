# ifndef REQUEST_HPP
# define REQUEST_HPP

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
		std::string							query;
		std::map<std::string, std::string>	heads;
		size_t								max_body_size;
		bool								stop; // if true stop !

	public:
		Request( void );
		Request( const Request &old );
		Request	&operator=( const Request &old );

		void	setMethod( std::string m );
		void	setPath( std::string p );
		void	setVersion( std::string v );
		void	setQuery( std::string q );
		void	setHeaders( std::map<std::string, std::string> h );
		void	setMaxBodySize( size_t m_b_s );
		void	setStop( bool s );

		std::string							getMethod( void ) const;
		std::string							getPath( void ) const;
		std::string							getVersion( void ) const;
		std::string							getQuery( void ) const;
		std::map<std::string, std::string>	getHeaders( void ) const;
		size_t								getMaxBodySize( void ) const;
		bool								getStop( void ) const;

		void	print_heads( void );
		~Request( void );

};

# endif
