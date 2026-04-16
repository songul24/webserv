#include "configfile.hpp"
#include <iostream>

int main(int argc, char **argv) {
    try {
        std::string configPath = "config.conf"; 
        if (argc == 2)
            configPath = argv[1];
        else if (argc > 2) {
            std::cerr << "Usage: ./webserv [config_file]" << std::endl;
            return 1;
        }

        Configfile config;
        std::string content = config.readFile(configPath);
        std::vector<std::string> tokens = config.tokenize(content);
        std::vector<ServerConfig> servers = config.parseServers(tokens);

        std::cout << "Number of servers: " << servers.size() << std::endl;

        for (size_t i = 0; i < servers.size(); i++) {
            std::cout << "\n--- Server " << i + 1 << " ---" << std::endl;

            std::cout << "Listen addresses:" << std::endl;
            for (size_t k = 0; k < servers[i].listens.size(); k++)
                std::cout << "  " << servers[i].listens[k].first
                          << ":" << servers[i].listens[k].second << std::endl;

            std::cout << "Server names: ";
            for (size_t k = 0; k < servers[i].server_names.size(); k++)
                std::cout << servers[i].server_names[k] << " ";
            std::cout << std::endl;

            std::cout << "Root: " << servers[i].root << std::endl;

            std::cout << "Index: ";
            for (size_t k = 0; k < servers[i].index.size(); k++)
                std::cout << servers[i].index[k] << " ";
            std::cout << std::endl;

            std::cout << "Max body size: " << servers[i].max_body_size << " bytes" << std::endl;
            std::cout << "Autoindex: " << (servers[i].autoindex ? "on" : "off") << std::endl;

            std::cout << "Methods: ";
            for (size_t j = 0; j < servers[i].methods.size(); j++)
                std::cout << servers[i].methods[j] << " ";
            std::cout << std::endl;

            if (!servers[i].upload.empty())
                std::cout << "Upload: " << servers[i].upload << std::endl;

            if (!servers[i].error_pages.empty()) {
                std::cout << "Error pages:" << std::endl;
                for (std::map<int, std::string>::const_iterator it = servers[i].error_pages.begin();
                     it != servers[i].error_pages.end(); ++it)
                    std::cout << "  " << it->first << " -> " << it->second << std::endl;
            }

            if (!servers[i].cgi.empty()) {
                std::cout << "CGI handlers:" << std::endl;
                for (std::map<std::string, std::string>::const_iterator it = servers[i].cgi.begin();
                     it != servers[i].cgi.end(); ++it)
                    std::cout << "  " << it->first << " -> " << it->second << std::endl;
            }

            for (size_t j = 0; j < servers[i].locations.size(); j++) {
                const LocationConfig& loc = servers[i].locations[j];
                std::cout << "\n  --- Location " << j + 1 << " ---" << std::endl;
                std::cout << "  Path: " << loc.path << std::endl;
                if (!loc.root.empty())
                    std::cout << "  Root: " << loc.root << std::endl;
                if (!loc.index.empty()) {
                    std::cout << "  Index: ";
                    for (size_t k = 0; k < loc.index.size(); k++)
                        std::cout << loc.index[k] << " ";
                    std::cout << std::endl;
                }
                if (!loc.methods.empty()) {
                    std::cout << "  Methods: ";
                    for (size_t k = 0; k < loc.methods.size(); k++)
                        std::cout << loc.methods[k] << " ";
                    std::cout << std::endl;
                }
                if (!loc.upload.empty())
                    std::cout << "  Upload: " << loc.upload << std::endl;
                if (!loc.redirect.empty())
                    std::cout << "  Redirect: " << loc.redirect_code << " -> " << loc.redirect << std::endl;
                std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
                if (!loc.cgi.empty()) {
                    std::cout << "  CGI handlers:" << std::endl;
                    for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin();
                         it != loc.cgi.end(); ++it)
                        std::cout << "    " << it->first << " -> " << it->second << std::endl;
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}