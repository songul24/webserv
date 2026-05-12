#include "../include/Request.hpp"

// ============================================================
//  GENERATE TEMP FILE NAME
// ============================================================

std::string generate_name( void )
{
	static int counter = 0;
	std::ostringstream oss;
	oss << "/tmp/body_" << getpid() << "_" << counter++;
	return oss.str();
}

// ============================================================
//  CONSTRUCTORS / DESTRUCTOR
// ============================================================

Request::Request( void ) :
	_method(""),
	_path(""),
	_query(""),
	_version(""),
	_body(""),
	_headers(),
	_max_body_size(0),
	_bytes_read(0),
	_status(START),
	_error(0),
	_file(-1)
{}

Request::Request( const Request &old ) :
	_method(old._method),
	_path(old._path),
	_query(old._query),
	_version(old._version),
	_body(old._body),
	_headers(old._headers),
	_max_body_size(old._max_body_size),
	_bytes_read(old._bytes_read),
	_status(old._status),
	_error(old._error),
	_file(old._file)
{}

Request &Request::operator=( const Request &old )
{
	if (this != &old)
	{
		// Fermer l'ancien fd si on en avait un
		if (_file != -1)
		{
			close(_file);
			_file = -1;
		}
		_method        = old._method;
		_path          = old._path;
		_query         = old._query;
		_version       = old._version;
		_body          = old._body;
		_headers       = old._headers;
		_max_body_size = old._max_body_size;
		_bytes_read    = old._bytes_read;
		_status        = old._status;
		_error         = old._error;
		_file          = old._file;
	}
	return (*this);
}

Request::~Request( void )
{
	// Fermer le fd s'il est encore ouvert
	if (_file != -1)
	{
		close(_file);
		_file = -1;
	}
}

// ============================================================
//  SETTERS
// ============================================================

void	Request::setMethod( std::string m )       { _method = m; }
void	Request::setPath( std::string p )         { _path = p; }
void	Request::setVersion( std::string v )      { _version = v; }
void	Request::setQuery( std::string q )        { _query = q; }
void	Request::setHeaders( std::map<std::string, std::string> h ) { _headers = h; }
void	Request::setMaxBodySize( size_t m )       { _max_body_size = m; }
void	Request::setStatus( t_status s )          { _status = s; }

void	Request::setBodyFile( std::string filename, int fd )
{
	_body = filename;
	_file = fd;
}

void	Request::addBytesRead( size_t n )
{
	_bytes_read += n;
}

void	Request::closeBodyFile( void )
{
	if (_file != -1)
	{
		close(_file);
		_file = -1;
	}
}

void	Request::setError( int code, std::string msg )
{
	if (!msg.empty())
		std::cerr << "Error " << code << ": " << msg << std::endl;
	// Nettoyer le fichier temporaire si erreur pendant body
	if (_file != -1)
	{
		close(_file);
		_file = -1;
		if (!_body.empty())
		{
			std::remove(_body.c_str());
			_body.clear();
		}
	}
	_error  = code;
	_status = ERROR;
}

// ============================================================
//  GETTERS
// ============================================================

std::string							Request::getMethod( void ) const  { return _method; }
std::string							Request::getPath( void ) const    { return _path; }
std::string							Request::getQuery( void ) const   { return _query; }
std::string							Request::getVersion( void ) const { return _version; }
std::string							Request::getBody( void ) const    { return _body; }
std::map<std::string, std::string>	Request::getHeaders( void ) const { return _headers; }
size_t								Request::getMaxBodySize( void ) const { return _max_body_size; }
size_t								Request::getBytesRead( void ) const { return _bytes_read; }
Request::t_status					Request::getStatus( void ) const  { return _status; }
int									Request::getError( void ) const   { return _error; }
int									Request::getFile( void ) const    { return _file; }
bool								Request::isComplete( void ) const { return _status == COMPLETE; }
bool								Request::isError( void ) const    { return _status == ERROR; }

// ============================================================
//  PRINT
// ============================================================

void	Request::print( void ) const
{
	std::cout << "========== REQUEST ==========\n";
	std::cout << "Method  : |" << _method  << "|\n";
	std::cout << "Path    : |" << _path    << "|\n";
	std::cout << "Query   : |" << _query   << "|\n";
	std::cout << "Version : |" << _version << "|\n";
	std::cout << "Headers :\n";
	for (std::map<std::string,std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		std::cout << "  [" << it->first << "] --> " << it->second << "\n";
	std::cout << "Body file : |" << _body << "|\n";
	std::cout << "Bytes read: " << _bytes_read << "\n";
	std::cout << "Status  : " << _status << " | Error: " << _error << "\n";
	std::cout << "=============================\n";
}

// ============================================================
//  STATIC HELPERS
// ============================================================

static std::vector<std::string> split( std::string &s, char sep )
{
	std::vector<std::string>	tokens;
	std::string					token;
	std::istringstream			ss(s);

	while (std::getline(ss, token, sep))
		tokens.push_back(token);
	return tokens;
}

static std::map<std::string, std::string> parse_headers_map( std::string &headers_str )
{
	std::map<std::string, std::string>	result;
	std::string							line;
	size_t								i = 0;

	while (i < headers_str.size())
	{
		line.clear();
		while (i < headers_str.size())
		{
			if (i + 1 < headers_str.size() && headers_str[i] == '\r' && headers_str[i + 1] == '\n')
			{
				i += 2;
				break ;
			}
			line += headers_str[i++];
		}
		if (line.empty())
			continue ;
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue ;
		std::string key   = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		size_t start = value.find_first_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start);
		result[key] = value;
	}
	return result;
}

static int	parse_method( std::string &method, Request &request )
{
	if (method != "GET" && method != "POST" && method != "DELETE")
		return (400);
	request.setMethod(method);
	return (0);
}

static int	parse_path( std::string &path, Request &request )
{
	if (path.empty() || path[0] != '/')
		return (400);
	if (path.find("..") != std::string::npos)
		return (403);
	if (path.size() > 2048)
		return (414);
	size_t pos = path.find('?');
	if (pos != std::string::npos)
	{
		request.setQuery(path.substr(pos + 1));
		request.setPath(path.substr(0, pos));
	}
	else
		request.setPath(path);
	return (0);
}

static int	parse_version( std::string &version, Request &request )
{
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return (400);
	if (version == "HTTP/1.0")
		return (505);
	request.setVersion(version);
	return (0);
}

static int	check_headers( Request &request )
{
	std::map<std::string, std::string> h = request.getHeaders();
	std::string method = request.getMethod();

	if (h.count("Transfer-Encoding"))
		return (501);
	if ((method == "GET" || method == "DELETE") && h.count("Content-Length"))
		return (400);
	if (method == "POST")
	{
		if (!h.count("Content-Length"))
			return (400);
		std::istringstream iss(h["Content-Length"]);
		size_t cl = 0;
		if (!(iss >> cl))
			return (400);
		if (cl > 0 && !h.count("Content-Type"))
			return (400);
		if (cl > request.getMaxBodySize() && request.getMaxBodySize() != 0)
			return (413);
	}
	return (0);
}

// ============================================================
//  SET BODY → écrit dans fichier temporaire chunk par chunk
// ============================================================

static void	set_body( std::string &chunk, Request &request )
{
	// Premier appel : créer le fichier temporaire
	if (request.getFile() == -1)
	{
		std::string filename = generate_name();
		int fd = open(filename.c_str(), O_CREAT | O_WRONLY, 0644);
		if (fd == -1)
		{
			request.setError(500, "Body file open failure");
			return ;
		}
		request.setBodyFile(filename, fd);
	}

	size_t bytes_read = request.getBytesRead();
	size_t max        = request.getMaxBodySize();

	// Trop de données
	if (bytes_read + chunk.size() > max)
	{
		request.setError(413, "Body larger than Content-Length");
		return ;
	}

	if (write(request.getFile(), chunk.c_str(), chunk.size()) == -1)
	{
		request.setError(500, "Body write() failure");
		return ;
	}

	request.addBytesRead(chunk.size());

	// Body complet
	if (request.getBytesRead() >= max)
	{
		request.closeBodyFile();
		request.setStatus(Request::COMPLETE);
	}
}

// ============================================================
//  POINT D'ENTREE PRINCIPAL
// ============================================================

void	parse_request( std::string &buffer, Request &request )
{
	// ---------- REQUEST LINE ----------
	if (request.getStatus() == Request::START || request.getStatus() == Request::REQUEST_LINE)
	{
		size_t pos = buffer.find(CRLF);
		if (pos == std::string::npos)
			return ;

		std::string request_line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		std::vector<std::string> tokens = split(request_line, ' ');
		if (tokens.size() != 3)
			return (request.setError(400, "Malformed request line"));

		int err = 0;
		if ((err = parse_method(tokens[0], request)) != 0)
			return (request.setError(err, "Bad method"));
		if ((err = parse_path(tokens[1], request)) != 0)
			return (request.setError(err, "Bad path"));
		if ((err = parse_version(tokens[2], request)) != 0)
			return (request.setError(err, "Bad version"));

		request.setStatus(Request::HEADERS);
	}

	// ---------- HEADERS ----------
	if (request.getStatus() == Request::HEADERS)
	{
		size_t pos = buffer.find("\r\n\r\n");
		if (pos == std::string::npos)
			return ;

		std::string headers_str = buffer.substr(0, pos);
		buffer.erase(0, pos + 4);

		request.setHeaders(parse_headers_map(headers_str));

		// Lire Content-Length → max_body_size
		std::map<std::string, std::string> h = request.getHeaders();
		if (h.count("Content-Length"))
		{
			std::istringstream iss(h["Content-Length"]);
			size_t cl = 0;
			iss >> cl;
			request.setMaxBodySize(cl);
		}

		int err = check_headers(request);
		if (err != 0)
			return (request.setError(err, "Header check failed"));

		if (request.getMethod() == "GET" || request.getMethod() == "DELETE")
			return (request.setStatus(Request::COMPLETE));

		if (request.getMaxBodySize() == 0)
			return (request.setStatus(Request::COMPLETE));

		request.setStatus(Request::BODY);
	}

	// ---------- BODY → fichier temporaire ----------
	if (request.getStatus() == Request::BODY)
	{
		if (!buffer.empty())
			set_body(buffer, request);
	}
}