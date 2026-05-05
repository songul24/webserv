#pragma once

#include "Request.hpp"


// void	get_method( std::string &buffer, Request &request );
// void	get_path( std::string &buffer, Request &request );
// void	get_version( std::string &buffer, Request &request );
void	                    parse_request( std::string &buffer, Request &request );
std::vector<std::string>	split(std::string &s, char separator);
int	                        parse_method( std::string &method, Request &request );
int	                        parse_path( std::string &path, Request &request );
int	                        parse_version( std::string &version, Request &request );
