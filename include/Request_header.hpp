#pragma once

#include "Request.hpp"

void								parse_headers( std::string &buffer, Request &request );
std::string							skip_request_line( std::string &buffer );
void								fill_lines( std::vector<std::string> &lines, std::string &buffer );
std::map<std::string, std::string>	cut_header( std::vector<std::string> &lines );
