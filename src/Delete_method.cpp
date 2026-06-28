#include "../include/WebServer.hpp"


int     checkURL(std::string& path)
{
        std::string url_path;
        struct stat buf;

        //check file exist
        if(stat(path.c_str(), &buf) != 0)
                return 404;
        if(access(path.c_str(), W_OK) != 0)
                return 403;
        if(S_ISDIR(buf.st_mode))
        {
                DIR *folder = opendir(path.c_str());
                if(!folder)
                        return 403;
                int st;
                for(struct dirent *folder_file = readdir(folder); folder_file != NULL; folder_file = readdir(folder))
                {
                        if (std::string(folder_file->d_name) == "." || std::string(folder_file->d_name) == "..")
                                continue;
                        if(path[path.size() - 1] == '/')
                                url_path = path + folder_file->d_name;
                        else
                                url_path = path + '/' + folder_file->d_name;
        
                        st = checkURL(url_path);
                        if(st != 204) 
                        {
                                closedir(folder);
                                return st;
                        }
                }
                if(closedir(folder) != 0)
                        return 403;
        }
        return 204;
}


int     deleteURL(std::string& path)
{
        std::string url_path;
        struct stat buf;
        if(stat(path.c_str(), &buf) != 0)
                return 404;
        if(access(path.c_str(), W_OK) != 0)
                return 403;
        if(S_ISDIR(buf.st_mode))
        {
                DIR *folder = opendir(path.c_str());
                if(!folder)
                        return 403;
                int st;
                for(struct dirent *folder_file = readdir(folder); folder_file != NULL; folder_file = readdir(folder))
                {
                        if (std::string(folder_file->d_name) == "." || std::string(folder_file->d_name) == "..")
                                continue;
                        if(path[path.size() - 1] == '/')
                                url_path = path + folder_file->d_name;
                        else
                                url_path = path + '/' + folder_file->d_name;
        
                        st = deleteURL(url_path);
                        if(st != 204) 
                        {
                                closedir(folder);
                                return st;
                        }
                }
                if(closedir(folder) != 0 || std::remove(path.c_str()) != 0)
                        return 403;
        }
        else if (S_ISREG(buf.st_mode))
        {
                if(std::remove(path.c_str()) != 0)
                        return 403;
        }
        return 204;
}



std::string    Delete_method(Connection& client, std::string& path, std::string& root)
{
        const ServerConfig config = client.getServer()->getConfig(); 
        int status =  checkURL(path);
        if(status != 204)
                return buildResponse(status, "", "text/html", &config);
                
        char resolved[PATH_MAX]; 
        char res_root[PATH_MAX]; 
        if(!realpath(path.c_str(), resolved) || !realpath(root.c_str(), res_root))
                return errorResponse(403, "text/html", &config);

        if(std::string(resolved).find(res_root) != 0)
                return errorResponse(403, "text/html", &config);

        return buildResponse(deleteURL(path), "", "text/html", &config);
}



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
                else if (loc.path.size() > 1 && loc.path[loc.path.size() - 1] == '/')
                {
                        if (url + "/" == loc.path)
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


bool is_method_allowed(const std::string& method, const std::vector<std::string>& methods)
{
        for (size_t i = 0; i < methods.size(); i++)
        {
                if (methods[i] == method)
                        return true;
        }
        return false;
}

