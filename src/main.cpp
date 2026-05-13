#include "../include/WebServer.hpp"


int     main(int argc, char **argv)
{
        std::string configPath = "config.conf"; 
        if (argc == 2)
                configPath = argv[1];
        else if (argc > 2)
        {
                std::cerr << "Usage: ./webserv [config_file]" << std::endl;
                return 1;
        }
        try {
                WebServer myserver;
                myserver.setupServer(configPath);
                std::srand(std::time(NULL));
                myserver.runServer();
        }
        catch(const std::exception& e)
        {
                std::cerr << e.what() << '\n';
        }
        return 0;
}