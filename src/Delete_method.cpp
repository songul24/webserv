#include "../include/WebServer.hpp"

std::string Delete_file(const std::string& url, const std::string& root);

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



// matching location for a URL
const LocationConfig* find_location(const ServerConfig& config, const std::string& url)
{
        const LocationConfig* best = NULL;
        size_t best_len = 0;

        for (size_t i = 0; i < config.locations.size(); i++)
        {
                const std::string& loc_path = config.locations[i].path;
                // Check if url starts with this location path
                if (url.find(loc_path) == 0)
                {
                        if (loc_path.length() > best_len)
                        {
                                best_len = loc_path.length();
                                best = &config.locations[i];
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


void  send_delete_response(Connection& client, const std::string& status)
{
        std::string body;
        if (status != "204 No Content")
                body = "<html><body><h1>" + status + "</h1></body></html>";

        std::string resp = "HTTP/1.0 " + status + "\r\n";
        std::ostringstream oss;
        oss << body.size();
        resp += "Content-Length: " + oss.str() + "\r\n";
        if (!body.empty())
                resp += "Content-Type: text/html\r\n";
        resp += "\r\n";
        resp += body;

        client.setResponse(resp);
        client.setRespLen(resp.size());
        client.setSentlen(0);
}


void    Deleth_method(Connection& client)
{
        std::string url    = client.getRequest().getPath();
        if (check_path(url))
                return send_delete_response(client, "403 Forbidden");

        const ServerConfig& config = client.getServer()->getConfig();   
        const LocationConfig* loc = find_location(config, url); 
        std::string status;
        
        const std::vector<std::string>& allowed = (loc && !loc->methods.empty()) ? loc->methods : config.methods;     
        if (!is_method_allowed("DELETE", allowed))
                return (send_delete_response(client, "405 Method Not Allowed"));

        // Pick the root: location first, fall back to server
        std::string root = (loc && !loc->root.empty()) ? loc->root : config.root;       
        return (send_delete_response(client, Delete_file(url, root)));
}