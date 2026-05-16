#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <string>
# include <vector>
# include <map>
# include <algorithm>
# include <sstream>
# include <cstdio>
# include <unistd.h>
# include <fcntl.h>

# define CRLF "\r\n"

std::string generate_name( void );

class Request
{
	public:
		typedef enum
		{
			START,
			REQUEST_LINE,
			HEADERS,
			BODY,
			COMPLETE,
			ERROR
		} t_status;

	private:
		std::string							method;
		std::string							path;
		std::string							query;
		std::string							version;
		std::string							body;		// nom du fichier temporaire
		std::map<std::string, std::string>	headers;
		size_t								max_body_size;
		size_t								bytes_read;
		t_status							status;
		int									error;
		int									file;		// fd fichier temporaire (-2 = pas ouvert)
		size_t								content_lenghth;

	public:
		Request( void );
		Request( const Request &old );
		Request &operator=( const Request &old );
		~Request( void );

		// Setters
		void	setMethod( std::string m );
		void	setPath( std::string p );
		void	setVersion( std::string v );
		void	setQuery( std::string q );
		void	setHeaders( std::map<std::string, std::string> h );
		void	setMaxBodySize( size_t m );
		void	setStatus( t_status s );
		void	setError( int code, std::string msg );
		void	setBodyFile( std::string filename, int fd );
		void	addBytesRead( size_t n );
		void	closeBodyFile( void );
		void	setcontent_lenghth( size_t cl );

		// Getters
		std::string							getMethod( void ) const;
		std::string							getPath( void ) const;
		std::string							getQuery( void ) const;
		std::string							getVersion( void ) const;
		std::string							getBody( void ) const;
		std::map<std::string, std::string>	getHeaders( void ) const;
		size_t								getMaxBodySize( void ) const;
		size_t								getBytesRead( void ) const;
		t_status							getStatus( void ) const;
		int									getError( void ) const;
		int									getFile( void ) const;
		size_t								getContentLength( void ) const;
		bool								isComplete( void ) const;
		bool								isError( void ) const;

		void	print( void ) const;
};

// Seule fonction publique de parsing
void	parse_request( std::string &buffer, Request &request );

# endif