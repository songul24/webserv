# Webserv (C++98)

A high-performance, non-blocking HTTP/1.1 web server implemented in C++98. This project leverages I/O multiplexing via `epoll` to handle multiple concurrent connections efficiently.

## Table of Contents
* [Overview](#overview)
* [Features](#features)
* [Installation & Usage](#installation--usage)
* [The I/O Model](#the-io-model)
* [Configuration System](#configuration-system)
* [Project Architecture](#project-architecture)
* [Testing](#testing)

---

## Overview
Webserv is a custom-built HTTP server designed to mimic the core functionality of Nginx. Written strictly in **C++98**, it emphasizes manual resource management, socket programming, and event-driven architecture.

## Features
* **Multiplexing:** Single-process event loop using `epoll` (Linux).
* **HTTP Methods:** Full support for `GET`, `POST`, and `DELETE`.
* **CGI:** Execution of scripts (Python, PHP, Bash) with support for environment variables.
* **Static Serving:** Serves HTML, CSS, and media files with `autoindex` support for directory listing.
* **File Uploads:** Integrated POST handling for saving files to the server.
* **Custom Config:** Flexible configuration syntax allowing multiple virtual hosts and location-specific rules.
* **Resilience:** Non-blocking sockets, timeout management, and graceful signal handling.

---

## Installation & Usage

### 1. Build the Project
The project uses a standard `Makefile`.
```bash
# Clone the repository
git clone <your-repo-link>
cd webserv

# Compile the executable
make
2. Launch the ServerBash# Run with the default configuration
./webserv

# Run with a custom configuration file
./webserv ./config/custom.conf
3. Build CommandsCommandActionmakeCompiles the webserv binary.make cleanRemoves object files.make fcleanRemoves objects and the binary.make rePerforms a full re-compile.make debugCompiles with debug symbols and runs via Valgrind.The I/O ModelWebserv utilizes a Non-blocking Event Loop. Unlike a multi-threaded server that spawns a thread per request, Webserv monitors all file descriptors (sockets) simultaneously.Initialization: Sockets are created, bound to ports, and set to O_NONBLOCK.Registration: Listening sockets are added to an epoll instance.The Loop: epoll_wait() pauses execution until an event (Read/Write) occurs.Handling:New Connection: accept() the client and add their FD to the event monitor.Data In (Read): Read request chunks, parse headers, and buffer the body.Data Out (Write): Send the prepared response. Large files are sent in chunks to keep the server responsive.Timeouts: A specialized monitor tracks "last activity" timestamps to drop stalled connections and prevent resource leaks.Configuration SystemThe server is configured via a .conf file. The syntax follows a block-based structure:Nginxserver {
    listen 127.0.0.1:8080;
    server_name example.com;
    client_max_body_size 10M;

    # Default Error Pages
    error_page 404 ./errors/404.html;

    location / {
        root ./var/www/html;
        index index.html;
        methods GET;
    }

    location /cgi-bin {
        root ./var/www/cgi;
        cgi .py /usr/bin/python3;
        methods GET POST;
    }

    location /uploads {
        root ./var/www/uploads;
        methods POST DELETE;
        upload ./var/www/uploads/storage;
        autoindex on;
    }
}
Project ArchitectureCore ComponentsWebServer: The "Orchestrator." It initializes the epoll instance and runs the main loop.ServerConfig: The "Brain." It parses the configuration file and stores rules for routing.Connection: The "Worker." Each instance represents a unique client session, managing its own state (Parsing -> Processing -> Sending).Request & Response: Data structures that encapsulate HTTP protocol details.Methods (GET/POST/DELETE): Specialized logic for handling file retrieval, CGI execution, and file deletion.File StructurePlaintext├── includes/           # Header files (.hpp)
├── src/                # Implementation files (.cpp)
│   ├── core/           # Main loop and WebServer logic
│   ├── config/         # Parsing and Config classes
│   ├── http/           # Request/Response parsing & Methods
│   └── utils/          # Error handling and File helpers
├── config/             # Configuration templates
├── var/www/            # Default web root & CGI scripts
└── Makefile
TestingYou can test the server using any standard browser or CLI tools:Basic GET:Bashcurl -v [http://127.0.0.1:8080/](http://127.0.0.1:8080/)
POST File Upload:Bashcurl -X POST -F "data=@myfile.txt" [http://127.0.0.1:8080/uploads/](http://127.0.0.1:8080/uploads/)
DELETE Request:Bashcurl -X DELETE [http://127.0.0.1:8080/uploads/myfile.txt](http://127.0.0.1:8080/uploads/myfile.txt)
Contributors[Your Name] - [GitHub Link][Partner Name] - [GitHub Link]