#include "../include/Request_header.hpp"

std::string skip_request_line(std::string &buffer)
{
	size_t pos = buffer.find("\r\n");
	if (pos == std::string::npos)
		return ("");

	buffer.erase(0, pos + 2);
	return (buffer);
}

static std::string trim(const std::string &str)
{
	size_t start = str.find_first_not_of(" \t");
	size_t end = str.find_last_not_of(" \t");

	if (start == std::string::npos)
		return ("");

	return (str.substr(start, end - start + 1));
}

static void to_lower(std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
		str[i] = std::tolower(str[i]);
}

std::map<std::string, std::string> cut_header(std::vector<std::string> &lines)
{
	std::map<std::string, std::string> header;

	for (size_t i = 0; i < lines.size(); i++)
	{
		const std::string &line = lines[i];

		size_t colon_index = line.find(':');
		if (colon_index == std::string::npos)
			continue;

		std::string key = line.substr(0, colon_index);
		std::string value = line.substr(colon_index + 1);

		key = trim(key);
		value = trim(value);

		to_lower(key);

		header[key] = value;
	}

	return header;
}

void fill_lines(std::vector<std::string> &lines, const std::string &buffer)
{
	size_t start = 0;

	while (true)
	{
		size_t end = buffer.find("\r\n", start);
		if (end == std::string::npos)
			break;

		if (end == start)
			break;

		lines.push_back(buffer.substr(start, end - start));
		start = end + 2;
	}
}

void parse_headers(std::string &buffer, Request &request)
{
	std::string new_buffer = skip_request_line(buffer);

	if (new_buffer.empty())
		return;

	std::vector<std::string> lines;
	fill_lines(lines, new_buffer);

	std::map<std::string, std::string> header = cut_header(lines);

	request.setHeaders(header);

	request.print_heads();
}