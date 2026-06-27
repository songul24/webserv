#include "../include/WebServer.hpp"

std::string     getExtention(const std::string& type)
{
        std::string     ext;

        if(type == "image/png" || type == "image/gif" || type == "image/webp" 
            || type == "image/jpeg" || type == "video/mp4")
                ext = "." + type.substr(6);
        else if( type == "text/plain" || type == "application/x-www-form-urlencoded")
                ext =  ".txt";
        else if (type == "application/octet-stream")
                ext = ".bin";
        else if (type == "text/html" || type == "text/css")
                ext = "." + type.substr(5);
        else if (type == "text/markdown")
		ext = ".md";
	else if (type == "application/pdf")
		ext = ".pdf";
        else if (type == "application/json")
		ext = ".json";
	else if (type == "text/x-csrc" )
		ext = ".c";
        return ext;
}

std::string     generateRandom_name()
{
        static int counter = 0;
        std::ostringstream oss;
        oss << getpid() << "_" << counter++;
        return oss.str();
}


std::string    Post_method(Connection &cnx, const LocationConfig* loc, std::string& path)
{
        
        ServerConfig conf = cnx.getServer()->getConfig();
        if (!cnx.getIsThereBody())
                return errorResponse(400, "text/html", &conf);
        Request req = cnx.getRequest();
        //get extention
        std::string extention = getExtention(req.getHeaders()["content-type"]);
        if(extention.empty())
        {
                extention = ".bin"; 
                // std::remove(req.getBody().c_str());
                // return errorResponse(415, "text/html", &conf);
        }
       
        std::string root   = loc->root.empty() ? conf.root :loc->root;
        std::string upload = loc->upload.empty() ? conf.upload : loc->upload;
        if (!root.empty() && root[root.size() - 1] == '/' && !upload.empty() && upload[0] == '/')
                        upload = upload.substr(1);
        else if (!root.empty() && root[root.size() - 1] != '/' && !upload.empty() && upload[0] != '/')
                        root += '/';

        std::string upload_path = root + upload;
        struct stat buf;
        if(!stat(upload_path.c_str(), &buf) && S_ISDIR(buf.st_mode))
        {
                std::string filename = generateRandom_name() + extention;
                upload_path += "/" + filename;
                if(std::rename(req.getBody().c_str(), upload_path.c_str()))
                {
                        std::cerr << "rename failed: " << strerror(errno)
              << " | src: " << req.getBody()
              << " | dst: " << upload_path << std::endl;
                        std::remove(req.getBody().c_str());
                        return errorResponse(500, "text/html", &conf);
                }
                std::string cgi_path = is_cgi(conf, loc, path);
                std::cerr << "path: [" << path << "]" << std::endl;
                if(stat(path.c_str(), &buf))
                {
                         std::remove(upload_path.c_str());
                        return errorResponse(404, "text/html", &conf);
                }
                else if(S_ISREG(buf.st_mode) && !cgi_path.empty())
                {
                        std::string resp = run_cgi(cgi_path, path, cnx, upload_path, loc);
                        std::remove(upload_path.c_str());
                        return resp;
                }
                else if(S_ISDIR(buf.st_mode))
                {
                        cgi_path.clear();
                        std::string mathced_script;
                        cgi_path = handle_dir_cgi(conf, loc, path, mathced_script);
                        if(!cgi_path.empty())
                        {
                                std::string resp = run_cgi(cgi_path, mathced_script, cnx, upload_path, loc);
                                std::remove(upload_path.c_str());
                                return resp;
                        }
                }
                return buildResponse(201, "File uploaded successfully", "text/html", NULL);

        }
        else
        {
                std::remove(req.getBody().c_str());
                return errorResponse(500, "text/html", NULL);
        }
}


