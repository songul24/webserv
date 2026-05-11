This project has been created as part of the 42 curriculum by zelgharb, machaouk.

██╗    ██╗███████╗██████╗ ███████╗███████╗██████╗ ██╗   ██╗
██║    ██║██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗██║   ██║
██║ █╗ ██║█████╗  ██████╔╝███████╗█████╗  ██████╔╝██║   ██║
██║███╗██║██╔══╝  ██╔══██╗╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝
╚███╔███╔╝███████╗██████╔╝███████║███████╗██║  ██║ ╚████╔╝
 ╚══╝╚══╝ ╚══════╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝

This is when you finally understand why URLs start with HTTP.


Table of Contents

Description
Instructions
Configuration
I/O Model
HTTP Methods
Project Layout
Quick Test Examples
Resources
Contributors


Description
Webserv is a fully functional HTTP/1.1 web server written from scratch in C++ 98, built as part of the 42 school curriculum.
The goal: understand how a real web server works at the lowest level — raw socket I/O, HTTP request parsing, non-blocking multiplexing, CGI execution, file uploads, and a custom NGINX-inspired configuration system. No frameworks, no Boost, no shortcuts.
The server is production-minded: non-blocking at all times, resilient to malformed requests, and configurable to serve multiple servers on multiple ports simultaneously.
FeatureStatus GET / POST / DELETE✅Static file serving✅File uploads✅Custom error pages✅Directory listing (autoindex)✅HTTP redirections✅CGI (PHP, Python, Shell)✅Multiple servers & ports✅Non-blocking I/O (epoll)✅Cookie support✅ (bonus)Session management✅ (bonus)Multiple CGI types✅ (bonus)

Instructions
Requirements

Linux or macOS
c++ compiler with C++98 support
php-cgi and/or python3 for CGI features

Build
bashgit clone <repo-url>
cd webserv
make
CommandDescriptionmakeCompile the projectmake reFull recompilemake cleanRemove object filesmake fcleanRemove objects + binarymake run ARGS="config/your.conf"Rebuild and run with a specific configmake debugRun under Valgrind
Run
bash# Default config (config/file.conf)
./webserv

# Custom config
./webserv config/your.conf

Configuration
Configuration uses an NGINX-inspired server { } block syntax. The default config path is config/file.conf.
nginxserver {
    listen               127.0.0.1:8080;
    client_max_body_size 10M;
    error_page           404 /errors/404.html;

    location / {
        root        ./var/www;
        index       index.html;
        methods     GET POST DELETE;
        autoindex   off;
    }

    location /upload {
        methods     POST;
        upload      ./var/www/uploads;
    }

    location /cgi-bin {
        methods     GET POST;
        cgi         .php /usr/bin/php-cgi;
        cgi         .py  /usr/bin/python3;
        cgi         .sh  /bin/bash;
    }

    location /old {
        return      301 /new;
    }
}
Directives
DirectiveDescriptionlisten <host>:<port>Bind address and portclient_max_body_size <size>Max request body (e.g. 10M)error_page <code> <path>Custom error pageroot <path>Filesystem root for the locationindex <file>Default file when path is a directorymethods GET|POST|DELETEAllowed HTTP methodsautoindex on|offEnable directory listingupload <dir>Upload destination directorycgi <ext> <interpreter>Map file extension to CGI interpreterreturn <code> <url>HTTP redirect

Configuration syntax is strictly validated — incorrect blocks will prevent the server from starting. Always use config/file.conf as a reference.


I/O Model & Multiplexing
Webserv uses Linux epoll for scalable, non-blocking I/O multiplexing.

A single epoll instance monitors all listening sockets and all active client connections simultaneously.
All sockets are set non-blocking (O_NONBLOCK + FD_CLOEXEC) — the main loop never blocks.
Event dispatch:

EPOLLIN on a listening socket → accept() new connection
EPOLLIN on a client socket → read & parse HTTP request
EPOLLOUT on a client socket → send response (chunked for large bodies)


Idle connections are reaped by periodic timeout checks (RECV_TIMEOUT / SEND_TIMEOUT).
SIGPIPE is ignored; SIGINT triggers a clean shutdown.


HTTP Methods & Behavior
GET

Serves static files from the location root.
For directory requests, tries index files in order.
If autoindex on and no index file found → returns a generated directory listing.
CGI-matched files (by extension) are executed; their stdout becomes the response.

POST

Writes the request body to a temp file, then moves it to the configured upload directory.
If the target is a CGI script, runs it with the body on stdin.
Content-Type determines the stored file extension.

DELETE

Recursively removes the target file or directory when permitted.
Returns 204, 403, 404, or 500 as appropriate.


Project Layout
webserv/
├── main.cpp                  # Entry point
├── Makefile
├── WebServer.cpp             # Core server, epoll loop
├── ServerConfig.cpp          # Config parser, socket setup
├── Connection.cpp            # Per-client request/response lifecycle
├── Request.cpp               # HTTP request parser
├── Methods.cpp               # GET logic, CGI, file utilities
├── MethodPost.cpp            # POST / upload logic
├── MethodDelete.cpp          # DELETE logic
├── runServer.cpp             # Main event loop (epoll, timeouts, signals)
├── MyError.cpp               # Custom exception class
├── includes/                 # Header files (.hpp)
├── config/
│   ├── file.conf             # Default config
│   └── *.conf                # Test/alternate configs
└── var/www/
    ├── index.html            # Static pages
    ├── errors/               # Custom error pages (400, 403, 404, 500 …)
    ├── session/              # Session management example (login, home)
    └── *.php / *.py / *.sh   # CGI scripts

Quick Test Examples
bash# Build & start with default config
make && ./webserv

# GET homepage
curl http://127.0.0.1:8080/

# POST file upload
curl -X POST -F "file=@test.txt" http://127.0.0.1:8080/upload/

# DELETE a file
curl -X DELETE http://127.0.0.1:8080/upload/test.txt

# Test Python CGI
curl http://127.0.0.1:8080/cgi-bin/script.py

# Raw HTTP via telnet
telnet 127.0.0.1 8080
GET / HTTP/1.1
Host: localhost
You can also test interactively with Postman or your browser at http://127.0.0.1:8080.

Resources
HTTP & Networking

RFC 7230 — HTTP/1.1 Message Syntax
RFC 7231 — HTTP/1.1 Semantics and Content
RFC 3875 — CGI/1.1 Specification
MDN Web Docs — HTTP
Beej's Guide to Network Programming
NGINX Documentation
Linux epoll man page
Linux poll man page

AI Usage
AI tools (primarily Claude) were used during this project for:

RFC comprehension — summarising dense RFC sections into plain English to identify relevant edge cases faster.
Debugging — describing unexpected server behaviours and using AI to suggest potential root causes, which were then verified manually with telnet and curl.
Session management design — getting an overview of common server-side session patterns, then adapting the approach to fit C++98 constraints and our CGI-based architecture.
README writing — drafting and structuring this file.

All AI-generated content was reviewed, understood, and validated by the team before inclusion. No code was blindly copy-pasted.

Contributors
songul24
zelgharb