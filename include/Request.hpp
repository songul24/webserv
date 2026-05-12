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
		std::string							_method;
		std::string							_path;
		std::string							_query;
		std::string							_version;
		std::string							_body;		// nom du fichier temporaire
		std::map<std::string, std::string>	_headers;
		size_t								_max_body_size;
		size_t								_bytes_read;
		t_status							_status;
		int									_error;
		int									_file;		// fd fichier temporaire

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
		bool								isComplete( void ) const;
		bool								isError( void ) const;

		void	print( void ) const;
};

// Seule fonction publique de parsing
void	parse_request( std::string &buffer, Request &request );

# endif