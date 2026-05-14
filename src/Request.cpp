#include "../include/Request.hpp"


std::string generate_name( void )
{
	static int counter = 0;
	std::ostringstream oss;
	oss << "/tmp/body_" << getpid() << "_" << counter++;
	return oss.str();
}


Request::Request( void ) :
	method(""),
	path(""),
	query(""),
	version(""),
	body(""),
	headers(),
	max_body_size(0),
	bytes_read(0),
	status(START),
	error(0),
	file(-2)
{}

Request::Request( const Request &old ) :
	method(old.method),
	path(old.path),
	query(old.query),
	version(old.version),
	body(old.body),
	headers(old.headers),
	max_body_size(old.max_body_size),
	bytes_read(old.bytes_read),
	status(old.status),
	error(old.error),
	file(-2)
{}

Request &Request::operator=( const Request &old )
{
	if (this != &old)
	{
		// Fermer l'ancien fd si on en avait un
		if (file != -2)
		{
			close(file);
			file = -2;
		}
		method        = old.method;
		path          = old.path;
		query         = old.query;
		version       = old.version;
		body          = old.body;
		headers       = old.headers;
		max_body_size = old.max_body_size;
		bytes_read    = old.bytes_read;
		status        = old.status;
		error         = old.error;
		file          = old.file;
	}
	return (*this);
}

Request::~Request( void )
{
	// Fermer le fd s'il est encore ouvert
	if (file != -2)
	{
		close(file);
		file = -2;
	}
}


void	Request::setMethod( std::string m )       { method = m; }
void	Request::setPath( std::string p )         { path = p; }
void	Request::setVersion( std::string v )      { version = v; }
void	Request::setQuery( std::string q )        { query = q; }
void	Request::setHeaders( std::map<std::string, std::string> h ) { headers = h; }
void	Request::setMaxBodySize( size_t m )       { max_body_size = m; }
void	Request::setStatus( t_status s )          { status = s; }

void	Request::setBodyFile( std::string filename, int fd )
{
	body = filename;
	file = fd;
}

void	Request::addBytesRead( size_t n )
{
	bytes_read += n;
}

void	Request::closeBodyFile( void )
{
	if (file != -2)
	{
		close(file);
		file = -2;
	}
}

void	Request::setError( int code, std::string msg )
{
	if (!msg.empty())
		std::cerr << "Error " << code << ": " << msg << std::endl;
	// Nettoyer le fichier temporaire si erreur pendant body
	if (file != -2)
	{
		close(file);
		file = -2;
		if (!body.empty())
		{
			std::remove(body.c_str());
			body.clear();
		}
	}
	error  = code;
	status = ERROR;
}


std::string							Request::getMethod( void ) const  { return method; }
std::string							Request::getPath( void ) const    { return path; }
std::string							Request::getQuery( void ) const   { return query; }
std::string							Request::getVersion( void ) const { return version; }
std::string							Request::getBody( void ) const    { return body; }
std::map<std::string, std::string>	Request::getHeaders( void ) const { return headers; }
size_t								Request::getMaxBodySize( void ) const { return max_body_size; }
size_t								Request::getBytesRead( void ) const { return bytes_read; }
Request::t_status					Request::getStatus( void ) const  { return status; }
int									Request::getError( void ) const   { return error; }
int									Request::getFile( void ) const    { return file; }
bool								Request::isComplete( void ) const { return status == COMPLETE; }
bool								Request::isError( void ) const    { return status == ERROR; }


void	Request::print( void ) const
{
	std::cout << "========== REQUEST ==========\n";
	std::cout << "Method  : |" << method  << "|\n";
	std::cout << "Path    : |" << path    << "|\n";
	std::cout << "Query   : |" << query   << "|\n";
	std::cout << "Version : |" << version << "|\n";
	std::cout << "Headers :\n";
	for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		std::cout << "  [" << it->first << "] --> " << it->second << "\n";
	std::cout << "Body file : |" << body << "|\n";
	std::cout << "Bytes read: " << bytes_read << "\n";
	std::cout << "Status  : " << status << " | Error: " << error << "\n";
	std::cout << "=============================\n";
}


std::vector<std::string> split( std::string &s, char sep )
{
	std::vector<std::string>	tokens;
	std::string					token;
	std::istringstream			ss(s);

	while (std::getline(ss, token, sep))
		tokens.push_back(token);
	return tokens;
}

 std::map<std::string, std::string> parseheaders_map( std::string &headers_str )
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
		for (size_t k = 0; k < key.size(); k++)
    		key[k] = std::tolower((unsigned char)key[k]);
		std::string value = line.substr(colon + 1);
		size_t start = value.find_first_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start);
		result[key] = value;
	}
	return result;
}

int	parsemethod( std::string &method, Request &request )
{
	if (method == "HEAD")
		method = "GET";// a verifier
	if (method != "GET" && method != "POST" && method != "DELETE")
		return (400);
	request.setMethod(method);
	return (0);
}

int	parsepath( std::string &path, Request &request )
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

int	parseversion( std::string &version, Request &request )
{
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return (400);
	// if (version == "HTTP/1.0")
	// 	return (505);
	request.setVersion(version);
	return (0);
}

int	checkheaders( Request &request )
{
	std::map<std::string, std::string> h = request.getHeaders();
	std::string method = request.getMethod();

	if (h.count("transfer-encoding"))
		return (501);
	if ((method == "GET" || method == "DELETE") && h.count("content-length"))
		return (400);
	if (method == "POST")
	{
		if (!h.count("content-length"))
		{
			// request.setMaxBodySize(0);
			return (0);
			// return (400);
		}
		std::istringstream iss(h["content-length"]);
		size_t cl = 0;
		if (!(iss >> cl))
			return (400);
		if (cl > 0 && !h.count("content-type"))
			return (400);
		if (cl > request.getMaxBodySize() && request.getMaxBodySize() != 0)
			return (413);
	}
	return (0);
}


void	setbody( std::string &chunk, Request &request )
{
	// Premier appel : créer le fichier temporaire
	if (request.getFile() == -2)
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


void	parse_request( std::string &buffer, Request &request )
{
	
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
		if ((err = parsemethod(tokens[0], request)) != 0)
			return (request.setError(err, "Bad method"));
		if ((err = parsepath(tokens[1], request)) != 0)
			return (request.setError(err, "Bad path"));
		if ((err = parseversion(tokens[2], request)) != 0)
			return (request.setError(err, "Bad version"));

		request.setStatus(Request::HEADERS);
	}

	
	if (request.getStatus() == Request::HEADERS)
	{
		size_t pos = buffer.find("\r\n\r\n");
		if (pos == std::string::npos)
			return ;

		std::string headers_str = buffer.substr(0, pos);
		buffer.erase(0, pos + 4);

		request.setHeaders(parseheaders_map(headers_str));

		
		std::map<std::string, std::string> h = request.getHeaders();
		if (h.count("Content-Length"))
		{
			std::istringstream iss(h["Content-Length"]);
			size_t cl = 0;
			iss >> cl;
			request.setMaxBodySize(cl);
		}

		int err = checkheaders(request);
		if (err != 0)
			return (request.setError(err, "Header check failed"));

		if (request.getMethod() == "GET" || request.getMethod() == "DELETE")
			return (request.setStatus(Request::COMPLETE));

		if (request.getMaxBodySize() == 0)
			return (request.setStatus(Request::COMPLETE));

		request.setStatus(Request::BODY);
	}

	
	if (request.getStatus() == Request::BODY)
	{
		if (!buffer.empty())
			setbody(buffer, request);
	}
}