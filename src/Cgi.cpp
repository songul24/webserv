#include "../include/WebServer.hpp"


std::string defaultBody(int code)
{
        switch(code)
        {
                case 400: return "<h1>400 Bad Request</h1>";
                case 403: return "<h1>403 Forbidden</h1>";
                case 404: return "<h1>404 Not Found</h1>";
                case 405: return "<h1>405 Method Not Allowed</h1>";
                case 413: return "<h1>413 Payload Too Large</h1>";
                case 415: return "<h1>415 Unsupported Media Type</h1>";
                case 500: return "<h1>500 Internal Server Error</h1>";
                case 502: return "<h1>502 Bad Gateway</h1>";
                case 504: return "<h1>504 Gateway Timeout</h1>";
                default:  return "<h1>Error</h1>";
        }
}

std::string errorResponse(int code, const std::string& type, const ServerConfig* conf)
{
        if (conf && conf->error_pages.count(code))
        {
                std::string page_path = conf->error_pages.at(code);
                if (exists(page_path))
                {
                        std::string content = read_File(page_path);
                        return buildResponse(code, content, type, NULL);
                }
        }
        return buildResponse(code, defaultBody(code), type, NULL);
}

std::string buildResponse(int code, const std::string &body, const std::string &type, const ServerConfig* conf)
{
        if (conf && conf->error_pages.count(code))
                return errorResponse(code, type, conf);
        std::string status;
        switch (code)
        {
                case 200: status = "200 OK"; break;
                case 201: status = "201 Created"; break;
                case 204: status = "204 No Content"; break;
                case 301: status = "301 Moved Permanently"; break;
                case 302: status = "302 Found"; break;
                case 304: status = "304 Not Modified"; break;
                case 400: status = "400 Bad Request"; break;
                case 401: status = "401 Unauthorized"; break;
                case 403: status = "403 Forbidden"; break;
                case 404: status = "404 Not Found"; break;
                case 405: status = "405 Method Not Allowed"; break;
                case 408: status = "408 Request Timeout"; break;
                case 409: status = "409 Conflict"; break;
                case 410: status = "410 Gone"; break;
                case 411: status = "411 Length Required"; break;
                case 413: status = "413 Payload Too Large"; break;
                case 414: status = "414 URI Too Long"; break;
                case 415: status = "415 Unsupported Media Type"; break;
                case 431: status = "431 Request Header Fields Too Large"; break;
                case 500: status = "500 Internal Server Error"; break;
                case 501: status = "501 Not Implemented"; break;
                case 502: status = "502 Bad Gateway"; break;
                case 503: status = "503 Service Unavailable"; break;
                case 504: status = "504 Gateway Timeout"; break;
                case 505: status = "505 HTTP Version Not Supported"; break;
                default:  status = "500 Internal Server Error"; break;
        }

        std::ostringstream res;
        res << "HTTP/1.0 " << status << "\r\nConnection: close\r\n";;
        res << "Content-Type: " << type << "\r\n";
        res << "Content-Length: " << body.size() << "\r\n";
        res << "\r\n";
        res << body;
        return res.str();
}


// std::string read_File(const std::string& path)
// {
//         std::ifstream file(path.c_str(), std::ios::binary);
//         if (!file.is_open())
//                 return "";
//         std::ostringstream ss;
//         ss << file.rdbuf();
//         return ss.str();
// }

std::string read_File(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    

    std::string content(size, '\0');
    if (file.read(&content[0], size))
        return content;
    
    return "";
}

std::string    is_cgi(const ServerConfig& srv, const LocationConfig* loc, const std::string& path)
{
        size_t dot = path.find_last_of('.');
        std::string ext;
        std::string cgiPath;

        if (dot != std::string::npos)
                ext = path.substr(dot);
        else
                ext = "";
        if(loc && loc->cgi.count(ext))
            cgiPath = loc->cgi.at(ext);
        else if (srv.cgi.count(ext))
            cgiPath = srv.cgi.at(ext);

        return cgiPath;
}


std::string handle_dir_cgi(const ServerConfig& srv, const LocationConfig* lc,std::string& path, std::string& script)
{
        std::vector<std::string> index;
        if (lc && !lc->index.empty())
                index = lc->index;
        else
                index = srv.index;
	if(path[path.size() - 1] != '/')
		path += '/';
        std::string full;
        std::string cgi_path;
	for (size_t i = 0; i < index.size(); i++)
        {
		full = path + index[i];
		if(exists(full))
                {
                        cgi_path = is_cgi(srv, lc, full);
                        if(!cgi_path.empty())
                        {
                                script = full;
			        return cgi_path;
                        }
                }
                full.clear();
        }
	return full;
}

void    kill_proccess(pid_t pid, int fd, const std::string& cgi)
{
        int status;
        kill(pid,SIGKILL) ;
	waitpid(pid,&status,0);
	close(fd);
	std::remove(cgi.c_str());
}

std::string     cgi_response(std::string& output, ServerConfig* conf)
{
        std::stringstream ss(output);
        std::string line, headers = "", status_line = "HTTP/1.0 200 OK\r\n";
        bool has_type = false, has_len = false;
        int code = 200;
        size_t bytes_read = 0;

        while (std::getline(ss, line))
        {
                if (line.empty() || line.find(':') == std::string::npos)
                        break;
        
                bytes_read += line.size() + 1;
                if (!line.empty() && line[line.size() - 1] == '\r')
                        line.erase(line.size() - 1);

                if (line.find("status:") == 0 || line.find("Status:") == 0)
                {
                    std::string val = line.substr(line.find(':') + 1);
                    if (!val.empty() && val[0] == ' ') val.erase(0, 1);
                    status_line = "HTTP/1.0 " + val + "\r\n";
                    code = atoi(val.c_str());
                } 
                else 
                {
                    if (line.find("Content-Type:") == 0 || line.find("Content-type:") == 0) has_type = true;
                    if (line.find("Content-Length:") == 0) has_len = true;
                    headers += line + "\r\n";
                }
        }

        if (code != 200 && code != 302)
                return errorResponse(code, "text/html", conf);

        std::string body = (output.size() > bytes_read) ? output.substr(bytes_read) : "";
        
        if (!has_type) headers += "Content-Type: text/html\r\n";
        if (!has_len)
        {
                std::stringstream sl;
                sl << body.size();
                headers += "Content-Length: " + sl.str() + "\r\n";
        }
        return status_line + "Connection: close\r\n" + headers + "\r\n" + body;
}



std::string    run_cgi(const std::string& cgiPath, std::string scriptPath, Connection& client, const std::string& bodyPath, const LocationConfig* loc)
{
        //scriptPath → the script that exists on server
        // bodyPath → the body/data the user sent, saved on disk
        // cgiPath → the interpreter to run the script with
        std::cerr << "CGIII: --------#######" << std::endl;
        ServerConfig conf = client.getServer()->getConfig(); 
        int cgi_fd, status;
        int body_fd = -1;
        std::string cgiOut = generateRandom_name();  // random name for output temp file
        cgi_fd   = open(cgiOut.c_str(),  O_CREAT | O_TRUNC | O_WRONLY, 0644); // create output/ STDOUT
        if(cgi_fd < 0)
                return buildResponse(500, "<h1>500</h1>", "text/html", &conf);

        if(!bodyPath.empty())
        {
                body_fd = open(bodyPath.c_str(), O_RDONLY);  // open uploaded file/ STDIN
                if(body_fd < 0)
                {
                        close(cgi_fd);
                        std::remove(cgiOut.c_str());
                        return buildResponse(500, "<h1>500</h1>", "text/html", &conf);
                }

        }

        pid_t pid = fork();
        if(pid == -1)
        {
                if(body_fd != -1) close(body_fd);
                close(cgi_fd);
                std::remove(cgiOut.c_str());
                return buildResponse(500, "<h1>500</h1>", "text/html", &conf);
        }
        if(pid == 0)
        {
                signal(SIGINT, SIG_IGN);
                std::map<std::string, std::string> envm = setCgiEnv(client, loc);
                char cwd[PATH_MAX];
                getcwd(cwd, sizeof(cwd));
                std::string script = scriptPath;
                if(script.size() >= 2 && script[0] == '.' && script[1] == '/')
                    script = script.substr(2);
                envm["SCRIPT_FILENAME"] = std::string(cwd) + "/" + script;
                char **envp = map_to_env(envm);
                char *args[3];
                args[0] = strdup(cgiPath.c_str());
                args[1] = strdup(scriptPath.c_str());
                args[2] = NULL;

                if(body_fd != -1)
                {
                        dup2(body_fd, STDIN_FILENO);
                        close(body_fd);
                }
                dup2(cgi_fd,   STDOUT_FILENO);
                close(cgi_fd);
                execve(cgiPath.c_str(), args, envp);
                free(args[0]);
                free(args[1]);
                free_env(envp);
                exit(1);
        }
        if(body_fd != -1) close(body_fd);

        time_t start = time(NULL);
        pid_t w = 0;
        while(w == 0 && (time(NULL) - start) < 7)
        {
                w = waitpid(pid, &status, WNOHANG);
                if(w == -1)
                {
                        kill_proccess(pid, cgi_fd, cgiOut);
                        return buildResponse(500, "<h1>500</h1>", "text/html", &conf);
                }
                else if (w == pid)
                {
                        close(cgi_fd);
                        if(WIFEXITED(status) && !WEXITSTATUS(status))
                        {
                                std::string output = read_File(cgiOut);
                                std::remove(cgiOut.c_str());
                                return (cgi_response(output, &conf));
                        }
                        std::remove(cgiOut.c_str());
                        return buildResponse(502, "<h1>502</h1>", "text/html", &conf);
                }
        }
        kill_proccess(pid, cgi_fd, cgiOut);
        return buildResponse(504, "<h1>504 Timeout</h1>", "text/html", &conf);

}
