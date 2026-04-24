#include "../include/Post_method.hpp"

void    set_enum( Type &type, std::string &content_type )
{
    if (content_type == "application/x-www-form-urlencoded")
        type = URLENCODED;
    else if (content_type.find("multipart/form-data"))
        type = MULTIPART;
    else
        type = OTHERS;
}

std::map<std::string, std::string>  urlencoded_handler( std::string &body )
{
    std::string                                     tmp;
    std::vector<std::string>                        holder;
    std::vector<std::string>::iterator              holder_it = holder.begin();
    std::map<std::string, std::string>              content;
    // std::map<std::string, std::string>::iterator    content_it = content.begiLn();

    int     i = 0;

    while (i < body.size() && i + 1 < body.size())
    {
        if (body.empty())
            return ;

        if (body[i] != '&')
        {
            tmp += body[i];
            i++;
        }
        else
        {
            holder.push_back(tmp);
            i++;
            continue ;
        }
    }

    std::string     data;
    std::string     key;
    std::string     value;
    i = 0;

    while (holder_it != holder.end())
    {
        data = *holder_it;
        while (data[i] != '=')
        {
            key += data[i];
            i++;
        }
        i++;
        while (i < data.size())
        {
            value += data[i];
        }
        content[key] = value;
        holder_it++;
    }

    return (content);
}

void    multipart_handler( std::string &body )
{
    
}

void    post_method( Request &request, Connection &cnx )
{
    // Multipart
    if (!cnx.getIsThereBody())
        return ;

    std::string body = request.getBody();
    std::string content_type = request.getHeaders()["content-type"];
    Type        _type;
    
    set_enum(_type, content_type);
    
    if (_type == URLENCODED)
        urlencoded_handler(body);
    else if (_type == MULTIPART)
        multipart_handler(body);
    else
        return_body(body);

}
