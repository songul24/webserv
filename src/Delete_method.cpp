#include "../include/WebServer.hpp"

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

std::string Delete_folder(const std::string& path)
{
        if(access(path.c_str(), W_OK) != 0)
                return (std::string("403 Forbidden"));
        DIR *folder = opendir(path.c_str());
        if(!folder)
                return (std::string("403 Forbidden"));
        for(struct dirent *folder_file = readdir(folder); folder_file != NULL; folder_file = readdir(folder))
        {
                if (std::string(folder_file->d_name) == "." || std::string(folder_file->d_name) == "..")
                        continue;
                std::string result = Delete_file(folder_file->d_name, path);
                if(result == std::string("403 Forbidden") || result == std::string("404 Not Found"))
                        return std::string("403 Forbidden");
        }
        if(closedir(folder) != 0)
                return (std::string("403 Forbidden"));
        if(std::remove(path.c_str()) != 0)
                return (std::string("403 Forbidden"));
        return (std::string("204 No Content"));
}

std::string Delete_file(const std::string& url, const std::string& root)
{
        if(url.empty() || root.empty() || check_path(url))
                return (std::string("404 Not Found"));
        std::string path;
        if(url[0] == '/')
                path = root + url;
        else
                path = root + '/' + url;
        struct stat buf;
        if(stat(path.c_str(), &buf) != 0)
                return (std::string("404 Not Found"));
        if (S_ISDIR(buf.st_mode))
                return Delete_folder(path);

        if(access(path.c_str(), W_OK) != 0)
                return (std::string("403 Forbidden"));
        
        if(std::remove(path.c_str()) != 0)
                return (std::string("403 Forbidden"));
        return (std::string("204 No Content"));
}



std::string Delete_mothod(const std::string& url, const std::string& root)
{       
        return Delete_file(url, root);
}