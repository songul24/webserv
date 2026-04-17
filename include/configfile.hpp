#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

struct LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> index;
    std::vector<std::string> methods;
    std::string upload;
    int redirect_code;
    std::string redirect;
    bool autoindex;
    std::map<std::string, std::string> cgi;

    LocationConfig() : path(""), root(""), index(), methods(), upload(""),
                       redirect_code(0), redirect(""), autoindex(false), cgi() {}
};

struct ServerConfig {
    int port;
    std::vector<std::pair<std::string, int> > listens;
    std::vector<std::string> server_names;
    std::string ip;
    std::string root;
    std::vector<std::string> index;
    size_t max_body_size;
    bool autoindex;
    std::vector<std::string> methods;
    std::string upload;
    std::map<int, std::string> error_pages;
    std::map<std::string, std::string> cgi;
    std::vector<LocationConfig> locations;

    ServerConfig() : port(80), listens(), server_names(), ip("0.0.0.0"), root(""),
                     index(), max_body_size(1 * 1024 * 1024), autoindex(false),
                     methods(), upload(""), error_pages(), cgi(), locations() {}
};

class Configfile {
public:
    Configfile();
    ~Configfile();

    std::string readFile(const std::string& filename);
    std::vector<std::string> tokenize(const std::string& content);
    std::vector<ServerConfig> parseServers(std::vector<std::string>& tokens);
    LocationConfig parseLocation(std::vector<std::string>& tokens, size_t& i);
    std::pair<std::string, int> parseListen(const std::string& listenValue);

private:
    size_t parseBodySize(const std::string& value);
    bool isValidMethod(const std::string& method);
    int validatePort(int port);
};

#endif