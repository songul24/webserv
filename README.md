# Webserv

A lightweight HTTP/1.1 web server written in C++98 as part of the 42 curriculum.

The project focuses on low-level networking, socket programming, HTTP request handling, CGI execution, and asynchronous I/O using `epoll`.

---

# Features

* HTTP/1.1 server
* Multiple server blocks
* Non-blocking sockets
* `epoll` multiplexing
* GET / POST / DELETE methods
* Static file serving
* File uploads
* CGI support (`.py`, `.sh`, `.php`)
* Autoindex
* Custom error pages
* Multiple ports & routes
* Config file parser inspired by Nginx

---

# Build & Run

## Compile

```bash
make
```

## Rebuild

```bash
make re
```

## Run with default config

```bash
./webserv
```

## Run with custom config

```bash
./webserv config/file.conf
```

---

# Configuration Example

```conf
server {
    listen 127.0.0.1:8080;
    client_max_body_size 50M;

    location / {
        root var/www;
        index index.html;
        methods GET POST DELETE;
        autoindex off;
    }

    location /cgi {
        root var/www/cgi;
        cgi .py /usr/bin/python3;
    }
}
```

---

# Architecture

The server uses:

* Non-blocking sockets (`O_NONBLOCK`)
* `epoll` for event handling
* One event loop to manage all clients
* Timeout handling for inactive connections

Main flow:

1. Accept connection
2. Read request
3. Parse HTTP message
4. Generate response
5. Send response
6. Close connection if needed

---

# HTTP Methods

## GET

* Serve static files
* Serve index pages
* Autoindex directories
* Execute CGI

## POST

* Handle uploads
* Execute CGI with body data

## DELETE

* Remove files/directories

---

# CGI

Supported interpreters:

* Python
* Bash
* PHP

Example:

```conf
cgi .py /usr/bin/python3;
```

The server forks a child process, executes the CGI, and sends its output as the HTTP response.

---

# Project Structure

```bash
.
├── main.cpp
├── WebServer.cpp
├── Connection.cpp
├── Request.cpp
├── Methods.cpp
├── MethodPost.cpp
├── MethodDelete.cpp
├── runServer.cpp
├── includes/
├── config/
├── errors/
└── var/www/
```

---

# Quick Tests

## GET

```bash
curl http://127.0.0.1:8080/
```

## POST Upload

```bash
curl -X POST -F "file=@test.txt" http://127.0.0.1:8080/upload
```

## DELETE

```bash
curl -X DELETE http://127.0.0.1:8080/file.txt
```

---

# Authors

* Fatimezzahra BBOT
* Chaimae Khater
