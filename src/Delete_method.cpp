#include "../include/WebServer.hpp"

std::string Delete_file(const std::string& url, const std::string& root, const ServerConfig& config);

bool    check_path(const std::string& url)
{
        if (url == "..")
                return true;
        if (url.find("../") == 0)
                return true;
        if (url.find("/../") != std::string::npos)
                return true;
        if (url.length() >= 3)
        {
                if (url.substr(url.length() - 3) == "/..") 
                        return true;
        }       
        return false;
}

std::string Delete_folder(const std::string& path, const ServerConfig& config)
{
        if(access(path.c_str(), W_OK) != 0)
                return errorResponse(403, "text/html", &config);
        DIR *folder = opendir(path.c_str());
        if(!folder)
                return errorResponse(403, "text/html", &config);
        for(struct dirent *folder_file = readdir(folder); folder_file != NULL; folder_file = readdir(folder))
        {
                if (std::string(folder_file->d_name) == "." || std::string(folder_file->d_name) == "..")
                        continue;
                std::string result = Delete_file(folder_file->d_name, path, config);
                if(result.find("HTTP/1.0 2") == std::string::npos) 
                {
                        closedir(folder);
                        return errorResponse(403, "text/html", &config);
                }
        }
        if(closedir(folder) != 0)
                return errorResponse(403, "text/html", &config);
        if(std::remove(path.c_str()) != 0)
                return errorResponse(403, "text/html", &config);
        return buildResponse(204, "", "text/html", NULL);
}

std::string Delete_file(const std::string& url, const std::string& root, const ServerConfig& config)
{
        if(url.empty() || root.empty() || check_path(url))
                return errorResponse(404, "text/html", &config);
        std::string path;
        if(url[0] == '/')
                path = root + url;
        else
                path = root + '/' + url;
        struct stat buf;
        if(stat(path.c_str(), &buf) != 0)
                return errorResponse(404, "text/html", &config);
        if (S_ISDIR(buf.st_mode))
                return Delete_folder(path, config);

        if(access(path.c_str(), W_OK) != 0)
                return errorResponse(403, "text/html", &config);
        
        if(std::remove(path.c_str()) != 0)
                return errorResponse(403, "text/html", &config);
        return buildResponse(204, "", "text/html", NULL);
}



// matching location for a URL
const LocationConfig* find_location(const ServerConfig& config, const std::string& url)
{
        const LocationConfig* best = NULL;
        size_t bestLen = 0;

        for (size_t i = 0; i < config.locations.size(); i++)
        {
                const LocationConfig &loc = config.locations[i];

                if (url.compare(0, loc.path.size(), loc.path) == 0)
                {
                        if (url.size() == loc.path.size() || url[loc.path.size()] == '/' ||
                                loc.path[loc.path.size()-1] == '/')
                        {
                                if (loc.path.size() > bestLen)
                                {
                                    bestLen = loc.path.size();
                                    best = &loc;
                                }
                        }
                }
        }
        return best;
}

//check if method is allowed in confg
bool is_method_allowed(const std::string& method, const std::vector<std::string>& methods)
{
        for (size_t i = 0; i < methods.size(); i++)
        {
                if (methods[i] == method)
                        return true;
        }
        return false;
}

std::string    Delete_method(Connection& client, const LocationConfig* loc, std::string& path)
{
        std::string url    = client.getRequest().getPath();
        const ServerConfig config = client.getServer()->getConfig();   
        if (check_path(url))
                return errorResponse(403, "text/html", &config);

        std::string root = (loc && !loc->root.empty()) ? loc->root : config.root;       
        return Delete_file(url, root, config);
}