#include "request_line.hpp"

std::vector<std::string>	split(std::string &s, char separator)
{
	std::vector<std::string>	tokens;
	std::string					token;
	std::istringstream			tokenStream(s);

	while(std::getline(tokenStream, token, separator))
		tokens.push_back(token);

	return (tokens);
}

int	parse_method( std::string &method, Request &request )
{
	if (method != "GET" && method != "POST" && method != "DELETE")
		return (400);
	else
	{
		request.setMethod(method);
		request.setStop(false);
		return (0);
	}
}

int	parse_path( std::string &path, Request &request )
{
	// /search?q=hello

	if (path[0] != '/')
		return (400);
	else if (path.find("..") != std::string::npos)
		return (403);

	size_t	pos = path.find('?');
	if (pos != std::string::npos)
	{
		request.setQuery(path.substr(pos + 1));
		request.setPath(path.substr(0, pos));
		request.setStop(false);
		return (0);
	}
	
	request.setPath(path);
	request.setStop(false);
	return (0);
}

int	parse_version( std::string &version, Request &request )
{
	if (version == "HTTP/1.1" || version == "HTTP/1.2")
		return (505);
	else if (version != "HTTP/1.0")
		return (400);
	else
	{
		request.setVersion(version);
		request.setStop(false);
		return (0);
	}
}


void	parse_request( std::string &buffer, Request &request )
{
	// (void)request;
	std::string		first;
	int				i = 0;

	while (buffer[i] && !(buffer[i] == '\r' && buffer[i + 1] == '\n'))
	{
		first += buffer[i];
		i++;
	}

	std::vector<std::string>			request_line = split(first, ' ');
	std::vector<std::string>::iterator	it = request_line.begin();

	if (request_line.size() != 3)
	{
		std::cout << "400 Bad Request" << std::endl;
		request.setStop(true);
		return ;
	}

	if (parse_method(*it, request) == 400)
	{
		std::cout << "400 Bad Request" << std::endl;
		request.setStop(true);
		std::cout << "parse_method\n";
		return ;
	}

	int	p_path = parse_path(*(it + 1), request);
	if (p_path == 400)
	{
		std::cout << "400 Bad Request" << std::endl;
		request.setStop(true);
		std::cout << "parse_path\n";

		return ;
	}
	else if (p_path == 403)
	{
		std::cout << "403 Forbidden" << std::endl;
		request.setStop(true);
		std::cout << "parse_path\n";
		return ;
	}

	int		p_version = parse_version(*(it + 2), request);
	if (p_version == 400)
	{
		std::cout << "400 Bad Request" << std::endl;
		request.setStop(true);
		std::cout << "parse_version\n";
		return ;
	}
	else if (p_version == 505)
	{
		std::cout << "505 HTTP Version Not Supported" << std::endl;
		request.setStop(true);
		std::cout << "parse_version\n";
		return ;
	}

	// std::cout << "Method -> " << request.getMethod() << std::endl;
	// std::cout << "Path -> " << request.getPath() << std::endl;
	// std::cout << "Version -> " << request.getVersion() << std::endl;
}
