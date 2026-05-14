#include "../include/WebServer.hpp"

bool exists(const std::string &path)
{
    struct stat st;
    int result = stat(path.c_str(), &st);
    if (result == 0)
        return true;
    else
        return false;
}

static bool isDir(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}


static std::string mimeType(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".txt") return "text/plain";
    if (ext == ".json") return "application/json";

    return "application/octet-stream";
}


static std::string autoIndex(const std::string &path,
                             const std::string &uri, const ServerConfig& conf)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return errorResponse(403, "text/html", &conf);
    std::vector<std::string> files;
    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        std::string name(entry->d_name);
        if (name == ".")//ne pas ignorer ".."
            continue;
        files.push_back(name);
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
    std::ostringstream html;
    html << "<html><body><h1>Index of " << uri << "</h1><ul>";
    for (size_t i = 0; i < files.size(); i++)
    {
        std::string link = uri;
        if (link[link.size() - 1] != '/')
            link += "/";
        link += files[i];

        html << "<li><a href=\"" << link << "\">"
             << files[i] << "</a></li>";
    }
    html << "</ul></body></html>";
    return buildResponse(200, html.str(), "text/html", NULL);
}

static std::string handleDirectory(const std::string &path, const std::string &uri, Connection& client, const LocationConfig* loc)
{
    ServerConfig srv = client.getServer()->getConfig();

    // Normaliser le path avec slash final pour la recherche des fichiers
    std::string dir_path = path;
    if (!dir_path.empty() && dir_path[dir_path.size() - 1] != '/')
        dir_path += "/";

    std::vector<std::string> index;
    if (loc != NULL && !loc->index.empty())
        index = loc->index;
    else
        index = srv.index;

    for (size_t i = 0; i < index.size(); i++)
    {
        std::string full = dir_path + index[i];
        if (exists(full))
        {
            std::string cgiPath = is_cgi(srv, loc, full);
            if (!cgiPath.empty())
                return run_cgi(cgiPath, full, client, "");
            return buildResponse(200, read_File(full), mimeType(full), NULL);
        }
    }

    bool autoindex;
    if (loc != NULL && loc->autoindex)
        autoindex = true;
    // else
    //     autoindex = srv.autoindex;

    if (autoindex)
        return autoIndex(dir_path, uri, srv);
    return errorResponse(403, "text/html", &srv);
}

std::string  Get_method(Connection &client)
{
    Request req = client.getRequest();
    ServerConfig srv = client.getServer()->getConfig(); 
    std::string uri = req.getPath();
    const LocationConfig *loc = find_location(srv, uri);
    const std::vector<std::string>& allowed = (loc && !loc->methods.empty()) ? loc->methods : srv.methods;     
    if (!is_method_allowed("GET", allowed))
            return errorResponse(405, "text/html", &srv);

    std::string root = (loc && !loc->root.empty()) ? loc->root : srv.root;
    if (root.empty())
        return errorResponse(500, "text/html", &srv);
    if (root[root.size() - 1] == '/')
        root.erase(root.size() - 1);
    if (uri.find("..") != std::string::npos)
        return errorResponse(403, "text/html", &srv);
    std::string path = root + uri;
    if (!exists(path))
        return errorResponse(404, "text/html", &srv);
    if (isDir(path))
        return handleDirectory(path, uri, client, loc);
    std::string cgiPath = is_cgi(srv, loc, path);
    if (!cgiPath.empty())
        return run_cgi(cgiPath, path, client, "");
    return buildResponse(200, read_File(path), mimeType(path), NULL);
}

















// #include "../include/WebServer.hpp"

// bool exists(const std::string &path)
// {
//     struct stat st;
//     int result = stat(path.c_str(), &st);
//     if (result == 0)
//         return true;
//     else
//         return false;
// }

// static bool isDir(const std::string &path)
// {
//     struct stat st;
//     if (stat(path.c_str(), &st) != 0)
//         return false;
//     return S_ISDIR(st.st_mode);
// }

// static std::string readFile(const std::string &path)
// {
//     std::ifstream file(path.c_str(), std::ios::binary);
//     if (!file.is_open())
//         return "";

//     std::ostringstream ss;
//     ss << file.rdbuf();
//     return ss.str();
// }

// static std::string mimeType(const std::string &path)
// {
//     size_t dot = path.find_last_of('.');
//     if (dot == std::string::npos)
//         return "application/octet-stream";

//     std::string ext = path.substr(dot);

//     if (ext == ".html" || ext == ".htm") return "text/html";
//     if (ext == ".css") return "text/css";
//     if (ext == ".js") return "application/javascript";
//     if (ext == ".png") return "image/png";
//     if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
//     if (ext == ".txt") return "text/plain";
//     if (ext == ".json") return "application/json";

//     return "application/octet-stream";
// }

// // std::string buildResponse(int code,
// //                                 const std::string &body,
// //                                 const std::string &type)
// // {
// //     std::string status;

// //     switch (code)
// //     {
// //         case 200: status = "200 OK"; break;
// //         case 301: status = "301 Moved Permanently"; break;
// //         case 403: status = "403 Forbidden"; break;
// //         case 404: status = "404 Not Found"; break;
// //         case 405: status = "405 Method Not Allowed"; break;
// //         case 500: status = "500 Internal Server Error"; break;
// //         default:  status = "500 Internal Server Error"; break;
// //     }

// //     std::ostringstream res;
// //     res << "HTTP/1.0 " << status << "\r\n";
// //     res << "Content-Type: " << type << "\r\n";
// //     res << "Content-Length: " << body.size() << "\r\n";
// //     res << "\r\n";
// //     res << body;

// //     return res.str();
// // }

// static const LocationConfig* matchLocation(const std::string &uri,
//                                            const ServerConfig &srv)
// {
//     const LocationConfig *best = NULL;
//     size_t bestLen = 0;

//     for (size_t i = 0; i < srv.locations.size(); i++)
//     {
//         const LocationConfig &loc = srv.locations[i];

//         if (uri.compare(0, loc.path.size(), loc.path) == 0)
//         {
//             if (loc.path.size() > bestLen)
//             {
//                 bestLen = loc.path.size();
//                 best = &loc;
//             }
//         }
//     }
//     return best;
// }

// static std::string autoIndex(const std::string &path,
//                              const std::string &uri)
// {
//     DIR *dir = opendir(path.c_str());
//     if (!dir)
//         return buildResponse(403, "<h1>403 Forbidden</h1>", "text/html");
//     std::vector<std::string> files;
//     struct dirent *entry;
//     while ((entry = readdir(dir)))
//     {
//         std::string name(entry->d_name);
//         if (name == ".")
//             continue;
//         files.push_back(name);
//     }
//     closedir(dir);
//     std::sort(files.begin(), files.end());
//     std::ostringstream html;
//     html << "<html><body><h1>Index of " << uri << "</h1><ul>";
//     for (size_t i = 0; i < files.size(); i++)
//     {
//         std::string link = uri;
//         if (link[link.size() - 1] != '/')
//             link += "/";
//         link += files[i];

//         html << "<li><a href=\"" << link << "\">"
//              << files[i] << "</a></li>";
//     }
//     html << "</ul></body></html>";
//     return buildResponse(200, html.str(), "text/html");
// }

// std::string executeCGI(const std::string& cgiPath, const std::string& scriptPath,
//                        const Request& req, const std::map<std::string, std::string>& env);
// static std::string handleDirectory(const std::string &path,
//                                    const std::string &uri,
//                                    const LocationConfig *loc,
//                                    const ServerConfig &srv,
//                                    const Request &req,
//                                    const std::map<std::string, std::string>& env)
// {
//     if (!uri.empty() && uri[uri.size() - 1] != '/')
//         return "HTTP/1.0 301 Moved Permanently\r\nLocation: " + uri + "/\r\nContent-Length: 0\r\n\r\n";
//     std::vector<std::string> index;
//     if (loc != NULL && !loc->index.empty()) {
//         index = loc->index;
//     } else {
//         index = srv.index;
//     }
//     for (size_t i = 0; i < index.size(); i++)
//     {
//         std::string full = path + "/" + index[i];
//         if (exists(full))
//         {
//             size_t dot = full.find_last_of('.');
//             std::string ext;

//             if (dot != std::string::npos)
//                 ext = full.substr(dot);
//             else
//                 ext = "";            
//             std::string cgiPath;
//             if (loc && loc->cgi.count(ext))
//                 cgiPath = loc->cgi.at(ext);
//             else if (srv.cgi.count(ext))
//                 cgiPath = srv.cgi.at(ext);
//             if (!cgiPath.empty())
//                 return executeCGI(cgiPath, full, req, env);
//             return buildResponse(200, readFile(full), mimeType(full));
//         }
//     }
//     bool autoindex;
//     if (loc != NULL) {
//         autoindex = loc->autoindex;
//     } else {
//         autoindex = srv.autoindex;
//     }

//     if (autoindex)
//         return autoIndex(path, uri);
//     return buildResponse(403, "<h1>403 Forbidden</h1>", "text/html");
// }

// std::string executeCGI(const std::string& cgiPath, const std::string& scriptPath,
//                        const Request& req, const std::map<std::string, std::string>& env)
// {
//     (void)req;    
//     int pipefd[2];
//     if (pipe(pipefd) == -1)
//         return buildResponse(500, "<h1>500 Pipe error</h1>", "text/html");
    
//     pid_t pid = fork();
    
//     if (pid == -1)
//     {
//         close(pipefd[0]);
//         close(pipefd[1]);
//         return buildResponse(500, "<h1>500 Fork error</h1>", "text/html");
//     }
//     if (pid == 0)
//     {
//         dup2(pipefd[1], STDOUT_FILENO);
//         close(pipefd[0]);
//         close(pipefd[1]);
//         char **envp = map_to_env(env);

//         char *args[] = {
//         (char*)cgiPath.c_str(),
//         (char*)scriptPath.c_str(),
//         NULL
//         };
//         execve(cgiPath.c_str(), args, envp);
//         free_env(envp);
//         exit(1);
//     }
//     else
//     {
//         close(pipefd[1]);
//         int status;
//         waitpid(pid, &status, 0);
//         std::string output;
//         char buffer[4096];
//         int bytes;
//         while ((bytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
//         {
//             buffer[bytes] = '\0';
//             output += buffer;
//         }
//         close(pipefd[0]);
//         if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
//             return buildResponse(500, "<h1>500 CGI execution failed</h1>", "text/html");
//         if (output.empty())
//             return buildResponse(500, "<h1>500 CGI returned empty</h1>", "text/html");
        
//         return buildResponse(200, output, "text/html");
//     }
// }

// std::string handleGET(const Request &req, const ServerConfig &srv, 
//                       const std::map<std::string, std::string>& env)
// {
//     std::string uri = req.getPath();
//     const LocationConfig *loc = matchLocation(uri, srv);
//     if (loc && !loc->methods.empty())
//     {
//         bool ok = false;
//         for (size_t i = 0; i < loc->methods.size(); i++)
//             if (loc->methods[i] == "GET")
//                 ok = true;
//         if (!ok)
//             return buildResponse(405, "<h1>405 Method Not Allowed</h1>", "text/html");
//     }
//     std::string root = (loc && !loc->root.empty()) ? loc->root : srv.root;
//     if (root.empty())
//         return buildResponse(500, "<h1>No root defined</h1>", "text/html");
//     if (root[root.size() - 1] == '/')
//         root.erase(root.size() - 1);
//     std::string path = root + uri;
//     if (uri.find("..") != std::string::npos)
//         return buildResponse(403, "<h1>403 Forbidden</h1>", "text/html");
//     if (!exists(path))
//         return buildResponse(404, "<h1>404 Not Found</h1>", "text/html");
//     if (isDir(path))
//         return handleDirectory(path, uri, loc, srv, req, env);
//     size_t dot = path.find_last_of('.');
//     std::string ext;

//     if (dot != std::string::npos)
//         ext = path.substr(dot);
//     else
//         ext = "";
//     std::string cgiPath;
//     if (loc && loc->cgi.count(ext))
//         cgiPath = loc->cgi.at(ext);
//     else if (srv.cgi.count(ext))
//         cgiPath = srv.cgi.at(ext);
//     if (!cgiPath.empty())
//     {
//         return executeCGI(cgiPath, path, req, env);
//     }
//     return buildResponse(200, readFile(path), mimeType(path));
// }
// // void    send_get_response(Connection& client, const std::string& status)
// // {
// //         // std::string body;
// //         // if (status != "204 No Content")
// //         //         body = "<html><body><h1>" + status + "</h1></body></html>";

// //         // std::string resp = "HTTP/1.0 " + status + "\r\n";
// //         // std::ostringstream oss;
// //         // oss << body.size();
// //         // resp += "Content-Length: " + oss.str() + "\r\n";
// //         // if (!body.empty())
// //         //         resp += "Content-Type: text/html\r\n";
// //         // resp += "\r\n";
// //         // resp += body;

// //         client.setResponse(resp);
// //         client.setRespLen(resp.size());
// //         client.setSentlen(0);
// // }

// // void Get_method(Connection &client, const std::map<std::string, std::string>& env)
// // {
// //     //Récupérer la requête
// //     Request req = client.getRequest();
// //     //Récupérer la configuration du serveur
// //     ServerConfig config = client.getServer()->getConfig();
// //     std::string response = handleGET(req, config, env);
// //     //Envoyer la réponse au client
    
// //     send_get_response(client, response);
    
// //     //Fermer la connexion (HTTP/1.0)
// // }
// void Get_method(Connection &client, const std::map<std::string, std::string>& env)
// {
//     Request req = client.getRequest();
//     ServerConfig config = client.getServer()->getConfig();

//     std::string response = handleGET(req, config, env);

//     client.setResponse(response);
//     client.setRespLen(response.size());
//     client.setSentlen(0);
// }


