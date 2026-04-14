#include "../include/WebServer.hpp"


std::string Delete_folder(const std::string& path)
{
        if(access(path.c_str(), W_OK) != 0)
                return (std::string("403 Forbidden"));
}

std::string Delete_mothod(const std::string& url, const std::string& root)
{
        if(url.empty() || url.find("..") != std::string::npos)
                return (std::string("404 Not found"));
        
        struct stat buf;
        std::string path = root + url;
        if(stat(path.c_str(), &buf) != 0)
        {
                if (S_ISDIR(buf.st_mode))
                        Delete_folder(path);
                return (std::string("403 Forbidden"));
        }

        if(access(path.c_str(), W_OK) != 0)
                return (std::string("403 Forbidden"));
        
        if(std::remove(path.c_str()) != 0)
                return (std::string("403 Forbidden"));
        return (std::string("204 No Content"));
}