#include "../include/Request_line.hpp"

void	get_method( std::string &buffer, Request &request )
{
	size_t		i = 0;

	std::string	token;

	while (i < buffer.size() && i + 1 < buffer.size() && buffer[i] != ' ')
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			return ;
		
		token += buffer[i];
		i++;
	}
	
	request.setMethod(token);
}

void	get_path( std::string &buffer, Request &request )
{
	size_t		i = 0;

	std::string	token;

	while (i < buffer.size() && i + 1 < buffer.size())
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			return ;
		
		if (buffer[i] != '/')
			i++;
		else
		{
			token += buffer[i];

			if (buffer[i + 1] == ' ')
				break ;

			i++;
		}
	}
	
	request.setPath(token);
}

void	get_version( std::string &buffer, Request &request )
{
	size_t		i = 0;
	int			count = 0;

	std::string	token;

	while (i < buffer.size() && i + 1 < buffer.size())
	{
		if (buffer[i] == ' ')
			count++;
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			break ;
		if (count == 2 && buffer[i] == 'H')
		{
			while (i < buffer.size() && i + 1 < buffer.size())
			{	
				token += buffer[i];
				
				if (buffer[i] == '\r' && buffer[i + 1] == '\n')
					break ;
				i++;
			}
		}	
		else
			i++;
	}
	request.setVersion(token);
}

std::vector<std::string>	split(std::tring &s, char separator)
{
	std::vector<std::string>	tokens;
	std::string					token;
	std::istringstream			tokenStream(s);

	while(std::getline(tokenStream, token, separator))
		tokens.push_back(token);

	return (tokens);
}

void	parse_request( std::string &buffer, Request &request )
{
	get_method(buffer, request);
	get_path(buffer, request);		
	get_version(buffer, request);

	std::string		first;
	int				i = 0;

	while (buffer[i] != '\r' && buffer[i + 1] != '\n')
	{
		first += buffer[i];
		i++;
	}

	std::vector<std::string>	request_line = split(first, ' ');

	 
	
	std::cout << "Method ----> " << request.getMethod() << std::endl;
	std::cout << "Path ----> " << request.getPath() << std::endl;
	std::cout << "Version ----> " << request.getVersion() << std::endl;
}
