#include "WebServer.hpp"


int     main(int argc, char **argv)
{
        (void)argc;
        (void)argv;
        try
        {
                WebServer myserver;
                myserver.setupServer();
                myserver.runServer();
        }
        catch(const std::exception& e)
        {
                std::cerr << e.what() << '\n';
        }
        return 0;
}