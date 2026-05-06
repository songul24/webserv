#include "../include/Request_line.hpp"

static std::vector<std::string> split( const std::string &s )
{
	std::vector<std::string>	tokens;
	std::istringstream 			stream(s);
	std::string					token;

	while (stream >> token)
		tokens.push_back(token);

	return (tokens);
}

static int parse_method( const std::string &method, Request &request )
{
	if (method != "GET" && method != "POST" && method != "DELETE")
		return (400);

	request.setMethod(method);
	return (0);
}

static int parse_path( const std::string &path, Request &request )
{
	if (path.empty() || path[0] != '/')
		return (400);

	if (path.find("..") != std::string::npos)
		return (403);

	size_t query_pos = path.find('?');

	if (query_pos != std::string::npos)
	{
		request.setPath(path.substr(0, query_pos));
		request.setQuery(path.substr(query_pos + 1));
	}
	else
	{
		request.setPath(path);
	}

	return (0);
}

static int parse_version( const std::string &version, Request &request )
{
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
		return (505);

	request.setVersion(version);
	return (0);
}

void parse_request( std::string &buffer, Request &request )
{
	size_t line_end = buffer.find("\r\n");

	if (line_end == std::string::npos)
	{
		request.setStop(true);
		return ;
	}

	std::string					first_line = buffer.substr(0, line_end);
	std::vector<std::string>	request_line = split(first_line);

	if (request_line.size() != 3)
	{
		std::cout << "400 Bad Request\n";
		request.setStop(true);
		return;
	}

	int							status;
	status = parse_method(request_line[0], request);

	if (status)
	{
		std::cout << "400 Bad Request\n";
		request.setStop(true);
		return;
	}

	status = parse_path(request_line[1], request);
	if (status == 400)
	{
		std::cout << "400 Bad Request\n";
		request.setStop(true);
		return;
	}
	else if (status == 403)
	{
		std::cout << "403 Forbidden\n";
		request.setStop(true);
		return;
	}

	status = parse_version(request_line[2], request);
	if (status == 505)
	{
		std::cout << "505 HTTP Version Not Supported\n";
		request.setStop(true);
		return;
	}
}
