#include "request_headers.hpp"

std::string	skip_request_line( std::string &buffer )
{
	size_t	count = 0;
	size_t	i = 0;

	while (i < buffer.size() && i++ < buffer.size())
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			break ;
		count++;
	}
	buffer.erase(0, count + 3);

	return (buffer);
}

std::map<std::string, std::string>	cut_header( std::vector<std::string> &lines )
{
	std::vector<std::string>::iterator	it = lines.begin();
	std::map<std::string, std::string>	header;
	std::string							line;
	std::string							key;
	std::string							value;
	size_t								colon_index = 0;

	while (it < lines.end())
	{
		line = *it;

		colon_index = line.find(':');
		if (colon_index == std::string::npos)
		{
			it++;
			continue;
		}

		key = line.substr(0, colon_index);
		value = line.substr(colon_index + 2);
		header[key] = value;
		it++;
	}

	return (header);
}

void	fill_lines( std::vector<std::string> &lines, std::string &buffer )
{
	size_t		i = 0;
	std::string	tmp;

	while (i < buffer.size() && i + 1 < buffer.size())
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
		{
			if (tmp.empty())
			{
				tmp.clear();
				i += 2;
				continue ;
			}
			else
			{
				lines.push_back(tmp);
				
				tmp.clear();
				i += 2;
				continue ;
			}
		}

		tmp += buffer[i];
		i++;
	}
}

void	parse_headers( std::string &buffer, Request &request )
{
	std::string							new_buffer;
	std::map<std::string, std::string>	header;
	std::vector<std::string>				lines;

	new_buffer = skip_request_line(buffer);

	if (new_buffer.empty()) 
		return ;

	fill_lines(lines, new_buffer);

	header = cut_header(lines);
	request.setHeaders(header);
	request.print_heads();

}
