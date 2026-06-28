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

    // Vidéo
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".webm") return "video/webm";
    if (ext == ".ogg") return "video/ogg";
    if (ext == ".avi") return "video/x-msvideo";
    if (ext == ".mov") return "video/quicktime";
    if (ext == ".mkv") return "video/x-matroska";
    
    // Audio (optionnel)
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".wav") return "audio/wav";
    
    // Image
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    
    // Texte
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".txt") return "text/plain";
    if (ext == ".json") return "application/json";
    
    // Fichiers téléchargeables
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";

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
    std::string dir_path;
    dir_path = path;
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
                return run_cgi(cgiPath, full, client, "", loc);
            return buildResponse(200, read_File(full), mimeType(full), NULL);
        }
    }

    bool autoindex = false;
    if (loc != NULL && loc->autoindex)
        autoindex = true;

    if (autoindex)
        return autoIndex(dir_path, uri, srv);
    return errorResponse(404, "text/html", &srv);
}

std::string  Get_method(Connection &client, const LocationConfig* loc, std::string& path)
{
    Request req = client.getRequest();
    ServerConfig srv = client.getServer()->getConfig(); 
    std::string uri = req.getPath();

    std::string root = (loc && !loc->root.empty()) ? loc->root : srv.root;
    if (root.empty())
        return errorResponse(500, "text/html", &srv);
    if (root[root.size() - 1] == '/')
        root.erase(root.size() - 1);
    if (uri.find("..") != std::string::npos)
        return errorResponse(403, "text/html", &srv);
    if (!exists(path))
        return errorResponse(404, "text/html", &srv);
    if (isDir(path))
        return handleDirectory(path, uri, client, loc);
    std::string cgiPath = is_cgi(srv, loc, path);
    if (!cgiPath.empty())
        return run_cgi(cgiPath, path, client, "", loc);
    return buildResponse(200, read_File(path), mimeType(path), NULL);
}
