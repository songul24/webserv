#include "../include/configfile.hpp"


Configfile::Configfile() {}
Configfile::~Configfile() {}

void configError(const std::string& message) {
    std::cerr << "Configuration error: " << message << std::endl;
    std::exit(1);
}
bool Configfile::isValidMethod(const std::string& method) {
    return (method == "GET" || method == "POST" || method == "DELETE");
}

int Configfile::validatePort(int port) {
    if (port <= 0 || port > 65535) {
        std::ostringstream oss;
        oss << port;
        configError("Invalid port number: " + oss.str());
    }
    return port;
}

size_t Configfile::parseBodySize(const std::string& value) {
    if (value.empty())
        configError("Invalid max_client_body_size value");

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
    for(size_t i = 0; i < digits.size(); i++) {
        if (!isdigit(digits[i]))
            configError("Invalid max_client_body_size value: " + value);
    }
    int n = std::atoi(digits.c_str());
    if (n <= 0)
        configError("Invalid max_client_body_size value: " + value);
    return (size_t)n * multiplier;
}

std::string Configfile::readFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        configError("Cannot open config file: " + filename);

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
        if (result.first != "0.0.0.0" && result.first != "localhost") {
        std::istringstream ss(result.first);
        std::string segment;
        int count = 0;
        bool valid = true;
        while (std::getline(ss, segment, '.')) {
            count++;

            if (segment.empty())
                valid = false;

            int num = std::atoi(segment.c_str());
            if (num < 0 || num > 255)
                valid = false;
        }

        if (!valid || count != 4) {
            configError("Invalid IP address: " + result.first);
        }
    }
        int port = std::atoi(listenValue.substr(colonPos + 1).c_str());
        result.second = validatePort(port);
    } else {
        result.first = "0.0.0.0";
        int port = std::atoi(listenValue.c_str());
        result.second = validatePort(port);
    }
    return result;
}

LocationConfig Configfile::parseLocation(std::vector<std::string>& tokens, size_t& i) {
    LocationConfig loc;

    i++;
    if (i < tokens.size() && tokens[i] != "{")
        loc.path = tokens[i++];
    else
        loc.path = "/";

    if (i >= tokens.size() || tokens[i] != "{")
        configError("Expected '{' after location path");
    i++;
    while (i < tokens.size() && tokens[i] != "}") {
        if (tokens[i] == "location")
            configError("Nested locations are not allowed");
        if (tokens[i] == "root") {
            i++;
            if (i >= tokens.size())
                configError("Invalid root in location");
            loc.root = tokens[i++];
            if (i >= tokens.size() || tokens[i] != ";")
                configError("Missing ; after root in location");
            i++;
        }
        else if (tokens[i] == "index") {
            i++;
            loc.index.clear();
            while (i < tokens.size() && tokens[i] != ";")
                loc.index.push_back(tokens[i++]);
            if (i >= tokens.size())
                configError("Missing ; after index in location");
            i++;
        }
        else if (tokens[i] == "allow") {
            i++;
            loc.methods.clear();
            while (i < tokens.size() && tokens[i] != ";") {
                std::string method = tokens[i++];
                if (!isValidMethod(method))
                    configError("Invalid HTTP method in location: " + method);
                loc.methods.push_back(method);
            }
            if (i >= tokens.size())
                configError("Missing ; after allow in location");
            i++;
        }
        else if (tokens[i] == "upload") {
            i++;
            if (i >= tokens.size())
                configError("Invalid upload directive in location");
            loc.upload = tokens[i++];
            if (i >= tokens.size() || tokens[i] != ";")
                configError("Missing ; after upload in location");
            i++;
        }
        else if (tokens[i] == "autoindex") {
            i++;
            if (i >= tokens.size())
                configError("Invalid autoindex in location");
            std::string val = tokens[i++];
            if (val != "on" && val != "off")
                configError("autoindex must be 'on' or 'off'");
            if (val == "on")
                loc.autoindex = true;
            else if (val == "off")
                loc.autoindex = false;
            else
                configError("autoindex must be 'on' or 'off'");
            if (i >= tokens.size() || tokens[i] != ";")
                configError("Missing ; after autoindex in location");
            i++;
        }
        else if (tokens[i] == "redirect") {
            i++;
            if (i + 1 >= tokens.size())
                configError("Invalid redirect directive in location");
            loc.redirect_code = std::atoi(tokens[i++].c_str());
            if (loc.redirect_code != 301 && loc.redirect_code != 302 &&
                loc.redirect_code != 303 && loc.redirect_code != 307 &&
                loc.redirect_code != 308)
                configError("Invalid redirect code");
            loc.redirect = tokens[i++];
            if (i >= tokens.size() || tokens[i] != ";")
                configError("Missing ; after redirect in location");
            i++;
        }
        else if (tokens[i] == "cgi") {
            i++;
            if (i + 1 >= tokens.size())
                configError("Invalid cgi directive in location");
            std::string extension = tokens[i++];
            std::string path = tokens[i++];
            loc.cgi[extension] = path;
            if (i >= tokens.size() || tokens[i] != ";")
                configError("Missing ; after cgi in location");
            i++;
        }
        else {
            std::cerr << "Warning: Unknown directive in location: " << tokens[i] << std::endl;
            while (i < tokens.size() && tokens[i] != ";") i++;//ignore until ;
            if (i < tokens.size()) i++;//skip ; for unknown directive 
        }
    }
    if (i >= tokens.size() || tokens[i] != "}")
        configError("Expected '}' at end of location block");
    i++;
    return loc;
}

void Configfile::validateServerConflicts(
    const ServerConfig& newServer,
    const std::vector<ServerConfig>& servers)
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        if (servers[i].ip == newServer.ip &&
            servers[i].port == newServer.port)
        {
            std::ostringstream oss;
            oss << "Duplicate server on same IP and port: "
                << newServer.ip << ":" << newServer.port;

            configError(oss.str());
        }
    }
}


std::vector<ServerConfig> Configfile::parseServers(std::vector<std::string>& tokens) {
    std::vector<ServerConfig> servers;
    size_t i = 0;

    while (i < tokens.size()) {

        if (tokens[i] != "server") {
            i++;
            continue;
        }
        i++;
        if (i >= tokens.size() || tokens[i] != "{")
            configError("Expected '{' after server");
        i++;
        ServerConfig server;
        while (i < tokens.size() && tokens[i] != "}") {
            if (tokens[i] == "listen") {
                i++;
                if (i >= tokens.size())
                    configError("Invalid listen directive");
                std::string listenValue = tokens[i++];
                std::pair<std::string, int> listenPair = parseListen(listenValue);
                for (size_t k = 0; k < server.listens.size(); k++) {
                    if (server.listens[k] == listenPair) {
                        configError("Duplicate listen directive: " + listenValue);
                    }
                }
                server.listens.push_back(listenPair);
                if (server.listens.size() == 1) {
                    server.ip = listenPair.first;
                    server.port = listenPair.second;
                }
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after listen");
                i++;
            }
            else if (tokens[i] == "server_name") {
                i++;
                server.server_names.clear();
                while (i < tokens.size() && tokens[i] != ";")
                    server.server_names.push_back(tokens[i++]);
                if (i >= tokens.size())
                    configError("Missing ; after server_name");
                i++; 
            }
            else if (tokens[i] == "root") {
                i++;
                if (i >= tokens.size())
                    configError("Invalid root");
                server.root = tokens[i++];
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after root");
                i++;
            }
            else if (tokens[i] == "index") {
                i++;
                server.index.clear();
                while (i < tokens.size() && tokens[i] != ";")
                    server.index.push_back(tokens[i++]);
                if (i >= tokens.size())
                    configError("Missing ; after index");
                i++; // skip ;
            }   
            else if (tokens[i] == "max_client_body_size") {
                i++;
                if (i >= tokens.size())
                    configError("Invalid max_client_body_size");
                server.max_body_size = parseBodySize(tokens[i++]);
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after max_client_body_size");
                i++; // skip ;
            }
            else if (tokens[i] == "allow") {
                i++;
                server.methods.clear();
                while (i < tokens.size() && tokens[i] != ";") {
                    std::string method = tokens[i++];
                    if (!isValidMethod(method))
                        configError("Invalid HTTP method: " + method);
                    server.methods.push_back(method);
                }
                if (i >= tokens.size())
                    configError("Missing ; after allow");
                i++;
            }
            else if (tokens[i] == "autoindex") {
                i++;
                if (i >= tokens.size())
                    configError("Invalid autoindex");
                std::string val = tokens[i++];
                if (val != "on" && val != "off")
                    configError("autoindex must be 'on' or 'off'");
                server.autoindex = (val == "on");
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after autoindex");
                i++;
            }
            else if (tokens[i] == "error_page") {
                i++;
                std::vector<int> codes;
                std::string path;
                while (i < tokens.size() && tokens[i] != ";") {
                    int n = std::atoi(tokens[i].c_str());
                    if (n >= 400 && n < 600) {
                        codes.push_back(n);
                        i++;
                    } else {
                        if (codes.empty()) {
                            configError("Invalid error_page directive: no error codes before path");
                        }
                        path = tokens[i];
                        i++;
                        break;
                    }
                }
                if (path.empty()) {
                    configError("Missing path for error_page directive");
                }
                if (i >= tokens.size() || tokens[i] != ";") {
                    configError("Missing ; after error_page");
                }
                for (size_t k = 0; k < codes.size(); k++) {
                    server.error_pages[codes[k]] = path;
                }

                i++;
            }
            else if (tokens[i] == "upload") {
                i++;
                if (i >= tokens.size())
                    configError("Invalid upload");
                server.upload = tokens[i++];
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after upload");
                i++;
            }
            else if (tokens[i] == "cgi") {
                i++;
                if (i + 1 >= tokens.size())
                    configError("Invalid cgi directive in server block");
                std::string extension = tokens[i++];
                std::string path = tokens[i++];
                server.cgi[extension] = path;
                if (i >= tokens.size() || tokens[i] != ";")
                    configError("Missing ; after cgi in server block");
                i++;
            }
            else if (tokens[i] == "location") {
                LocationConfig loc = parseLocation(tokens, i);
                for(size_t k = 0; k < server.locations.size(); k++) {
                    if (server.locations[k].path == loc.path) {
                        configError("Duplicate location path: " + loc.path);
                    }
                }
                server.locations.push_back(loc);
            }
            else {
                std::cerr << "Warning: Unknown directive: " << tokens[i] << std::endl;
                while (i < tokens.size() && tokens[i] != ";") i++;
                if (i < tokens.size()) i++;
            }
        }
        if (i >= tokens.size() || tokens[i] != "}")
                configError("Missing closing '}' for server block");

        if (server.listens.empty())
            configError("Server block has no listen directive");
        if (server.index.empty())
            configError("Server block must have at least one index file specified");
        i++;
        validateServerConflicts(server, servers);
        servers.push_back(server);
    }
    if (servers.empty())
        configError("No server blocks found in configuration");

    return servers;
}