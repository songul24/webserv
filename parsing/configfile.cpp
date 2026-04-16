#include "configfile.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

Configfile::Configfile() {}
Configfile::~Configfile() {}


bool Configfile::isValidMethod(const std::string& method) {
    return (method == "GET" || method == "POST" || method == "DELETE");
}

int Configfile::validatePort(int port) {
    if (port <= 0 || port > 65535) {
        std::ostringstream oss;
        oss << port;
        throw std::runtime_error("Invalid port number: " + oss.str());
    }
    return port;
}

size_t Configfile::parseBodySize(const std::string& value) {
    std::cout << "Parsing max_client_body_size: " << value << std::endl;
    if (value.empty())
        throw std::runtime_error("Invalid max_client_body_size value");

    std::string digits = value;
    size_t multiplier = 1;
    char last = value[value.size() - 1];

    if (last == 'M' || last == 'm') {
        multiplier = 1024 * 1024;
        digits = value.substr(0, value.size() - 1);
    } else if (last == 'K' || last == 'k') {
        multiplier = 1024;
        digits = value.substr(0, value.size() - 1);
    } else if (last == 'G' || last == 'g') {
        multiplier = 1024 * 1024 * 1024;
        digits = value.substr(0, value.size() - 1);
    }

    int n = std::atoi(digits.c_str());
    if (n <= 0)
        throw std::runtime_error("Invalid max_client_body_size value: " + value);

    return (size_t)n * multiplier;
}


std::string Configfile::readFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + filename);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


std::vector<std::string> Configfile::tokenize(const std::string& content) {
    std::vector<std::string> tokens;
    std::string current;
    bool inComment = false;

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];

        // Handle comments (# until end of line)
        if (c == '#') {
            inComment = true;
        }
        if (inComment) {
            if (c == '\n')
                inComment = false;
            continue;
        }

        if (isspace(c)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else if (c == '{' || c == '}' || c == ';') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
        } else {
            current += c;
        }
    }

    if (!current.empty())
        tokens.push_back(current);

    return tokens;
}


std::pair<std::string, int> Configfile::parseListen(const std::string& listenValue) {
    std::pair<std::string, int> result;
    size_t colonPos = listenValue.find(':');

    if (colonPos != std::string::npos) {
        result.first = listenValue.substr(0, colonPos);
        int port = std::atoi(listenValue.substr(colonPos + 1).c_str());
        result.second = validatePort(port);
    } else {
        result.first = "0.0.0.0"; // bind sur toutes les interfaces par défaut
        int port = std::atoi(listenValue.c_str());
        result.second = validatePort(port);
    }

    return result;
}


LocationConfig Configfile::parseLocation(std::vector<std::string>& tokens, size_t& i) {
    LocationConfig loc;

    i++; // skip "location"

    if (i < tokens.size() && tokens[i] != "{")
        loc.path = tokens[i++];
    else
        loc.path = "/";

    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("Expected '{' after location path");

    i++; // skip "{"

    while (i < tokens.size() && tokens[i] != "}") {

        
        if (tokens[i] == "root") {
            i++;
            if (i >= tokens.size())
                throw std::runtime_error("Invalid root in location");
            loc.root = tokens[i++];
            if (i >= tokens.size() || tokens[i++] != ";")
                throw std::runtime_error("Missing ; after root in location");
        }

        
        else if (tokens[i] == "index") {
            i++;
            loc.index.clear();
            while (i < tokens.size() && tokens[i] != ";")
                loc.index.push_back(tokens[i++]);
            if (i >= tokens.size())
                throw std::runtime_error("Missing ; after index in location");
            i++; // skip ;
        }

        
        else if (tokens[i] == "allow") {
            i++;
            loc.methods.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                std::string method = tokens[i++];
                if (!isValidMethod(method))
                    throw std::runtime_error("Invalid HTTP method in location: " + method);
                loc.methods.push_back(method);
            }
            if (i >= tokens.size())
                throw std::runtime_error("Missing ; after allow in location");
            i++; // skip ;
        }

        
        else if (tokens[i] == "upload") {
            i++;
            if (i >= tokens.size())
                throw std::runtime_error("Invalid upload in location");
            loc.upload = tokens[i++];
            if (i >= tokens.size() || tokens[i++] != ";")
                throw std::runtime_error("Missing ; after upload in location");
        }

        
        else if (tokens[i] == "autoindex") {
            i++;
            if (i >= tokens.size())
                throw std::runtime_error("Invalid autoindex in location");
            std::string val = tokens[i++];
            if (val != "on" && val != "off")
                throw std::runtime_error("autoindex must be 'on' or 'off'");
            loc.autoindex = (val == "on");
            if (i >= tokens.size() || tokens[i++] != ";")
                throw std::runtime_error("Missing ; after autoindex in location");
        }

        
        else if (tokens[i] == "redirect") {
            i++;
            if (i + 1 >= tokens.size())
                throw std::runtime_error("Invalid redirect directive in location");
            loc.redirect_code = std::atoi(tokens[i++].c_str());
            if (loc.redirect_code != 301 && loc.redirect_code != 302 &&
                loc.redirect_code != 303 && loc.redirect_code != 307 &&
                loc.redirect_code != 308)
                throw std::runtime_error("Invalid redirect code");
            loc.redirect = tokens[i++];
            if (i >= tokens.size() || tokens[i++] != ";")
                throw std::runtime_error("Missing ; after redirect in location");
        }

        
        else if (tokens[i] == "cgi") {
            i++;
            if (i + 1 >= tokens.size())
                throw std::runtime_error("Invalid cgi directive in location");
            std::string extension = tokens[i++];
            std::string path = tokens[i++];
            loc.cgi[extension] = path;
            if (i >= tokens.size() || tokens[i++] != ";")
                throw std::runtime_error("Missing ; after cgi in location");
        }

        
        else {
            std::cerr << "Warning: Unknown directive in location: " << tokens[i] << std::endl;
            while (i < tokens.size() && tokens[i] != ";") i++;
            if (i < tokens.size()) i++;
        }
    }

    if (i >= tokens.size() || tokens[i] != "}")
        throw std::runtime_error("Missing closing '}' for location");

    i++; // skip "}"
    return loc;
}


std::vector<ServerConfig> Configfile::parseServers(std::vector<std::string>& tokens) {
    std::vector<ServerConfig> servers;
    size_t i = 0;

    while (i < tokens.size()) {

        if (tokens[i] != "server") {
            i++;
            continue;
        }

        i++; // skip "server"

        if (i >= tokens.size() || tokens[i] != "{")
            throw std::runtime_error("Expected '{' after server");

        i++; // skip "{"

        ServerConfig server;

        while (i < tokens.size() && tokens[i] != "}") {

           
            if (tokens[i] == "listen") {
                i++;
                if (i >= tokens.size())
                    throw std::runtime_error("Invalid listen directive");

                std::string listenValue = tokens[i++];
                std::pair<std::string, int> listenPair = parseListen(listenValue);
                server.listens.push_back(listenPair);

                
                if (server.listens.size() == 1) {
                    server.ip = listenPair.first;
                    server.port = listenPair.second;
                }

                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ; after listen");
            }

            
            else if (tokens[i] == "server_name") {
                i++;
                server.server_names.clear();
                while (i < tokens.size() && tokens[i] != ";")
                    server.server_names.push_back(tokens[i++]);
                if (i >= tokens.size())
                    throw std::runtime_error("Missing ; after server_name");
                i++; 
            }

           
            else if (tokens[i] == "root") {
                i++;
                if (i >= tokens.size())
                    throw std::runtime_error("Invalid root");
                server.root = tokens[i++];
                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ; after root");
            }

            
            else if (tokens[i] == "index") {
                i++;
                server.index.clear();
                while (i < tokens.size() && tokens[i] != ";")
                    server.index.push_back(tokens[i++]);
                if (i >= tokens.size())
                    throw std::runtime_error("Missing ; after index");
                i++; // skip ;
            }

            
            else if (tokens[i] == "max_client_body_size") {
                i++;
                if (i >= tokens.size())
                    throw std::runtime_error("Invalid max_client_body_size");
                server.max_body_size = parseBodySize(tokens[i++]);
                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ; after max_client_body_size");
            }

           
            else if (tokens[i] == "allow") {
                i++;
                server.methods.clear();
                while (i < tokens.size() && tokens[i] != ";") {
                    std::string method = tokens[i++];
                    if (!isValidMethod(method))
                        throw std::runtime_error("Invalid HTTP method: " + method);
                    server.methods.push_back(method);
                }
                if (i >= tokens.size())
                    throw std::runtime_error("Missing ; after allow");
                i++; // skip ;
            }

           
            else if (tokens[i] == "autoindex") {
                i++;
                if (i >= tokens.size())
                    throw std::runtime_error("Invalid autoindex");
                std::string val = tokens[i++];
                if (val != "on" && val != "off")
                    throw std::runtime_error("autoindex must be 'on' or 'off'");
                server.autoindex = (val == "on");
                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ;");
            }

            
            else if (tokens[i] == "error_page") {
                i++;
                std::vector<int> codes;
                while (i < tokens.size() && tokens[i] != ";") {
                    int n = std::atoi(tokens[i].c_str());
                    if (n >= 400 && n < 600) {
                        codes.push_back(n);
                        i++;
                    } else {
                       
                        std::string path = tokens[i++];
                        for (size_t k = 0; k < codes.size(); k++)
                            server.error_pages[codes[k]] = path;
                        codes.clear();
                    }
                }
                if (i >= tokens.size())
                    throw std::runtime_error("Missing ; after error_page");
                i++; 
            }

           
            else if (tokens[i] == "upload") {
                i++;
                if (i >= tokens.size())
                    throw std::runtime_error("Invalid upload");
                server.upload = tokens[i++];
                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ; after upload");
            }

            
            else if (tokens[i] == "cgi") {
                i++;
                if (i + 1 >= tokens.size())
                    throw std::runtime_error("Invalid cgi directive");
                std::string extension = tokens[i++];
                std::string path = tokens[i++];
                server.cgi[extension] = path;
                if (i >= tokens.size() || tokens[i++] != ";")
                    throw std::runtime_error("Missing ; after cgi");
            }

            else if (tokens[i] == "location") {
                server.locations.push_back(parseLocation(tokens, i));
            }

            else {
                std::cerr << "Warning: Unknown directive: " << tokens[i] << std::endl;
                while (i < tokens.size() && tokens[i] != ";") i++;
                if (i < tokens.size()) i++;
            }
        }

        if (i >= tokens.size() || tokens[i] != "}")
            throw std::runtime_error("Missing closing '}' for server");

        if (server.listens.empty())
            throw std::runtime_error("Server block has no listen directive");

        i++; 
        servers.push_back(server);
    }

    if (servers.empty())
        throw std::runtime_error("No server block found in config file");

    return servers;
}