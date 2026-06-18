# Webserv

A lightweight HTTP/1.1 web server written in C++98 as part of the 42 curriculum by machaouk, zelgharb

The project focuses on low-level networking, socket programming, HTTP request handling, CGI execution, and asynchronous I/O using `epoll`.

---

# Features

* HTTP/1.1 server
* Multiple server blocks
* Non-blocking sockets
* `epoll` multiplexing
* GET / POST / DELETE methods
* File uploads
* CGI support (`.py`, `.php`, `.sh`)
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
    server_name 127.0.0.1;
    listen 127.0.0.1:8089;
    max_client_body_size 100M;
    root var/www;

    location / {
        root var/www;
        index index.html;
        allow GET POST DELETE;
    }

    location /upload {
        allow GET POST;
        root var/www;
        index index.html;
        autoindex on;
        upload /upload;
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
6. Close connection

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
* PHP
* BASH

Example:

```conf
cgi .py /usr/bin/python3;
```

The server forks a child process, executes the CGI, and sends its output as the HTTP response.

---

# Project Structure

```bash
.
├── Makefile
├── README.md
├──
├── config/
│
├── include/
│   ├── configfile.hpp
│   ├── Request.hpp
│   ├── Server.hpp
│   ├── WebServer.hpp
│   └── Connection.hpp
│
├── src/
│   ├── main.cpp
│   ├── WebServer.cpp
│   ├── Server.cpp
│   ├── Connection.cpp
│   ├── Request.cpp
│   ├── Configfile.cpp
│   ├── Get_methode.cpp
│   ├── Post_method.cpp
│   ├── Delete_method.cpp
│   ├── Cgi.cpp
│   └── utils.cpp
│
├── errors/
│   ├── 400.html
│   ├── 403.html
│   ├── 404.html
│   ├── 405.html
│   ├── 408.html
│   ├── 409.html
│   ├── 411.html
│   ├── 413.html
│   ├── 414.html
│   ├── 415.html
│   ├── 500.html
│   ├── 501.html
│   ├── 502.html
│   └── 504.html
│
└── var/www
    ├── session/
    ├── storage/
    ├── upload/
    ├── website1/
    ├── website2/
    ├── website3/
    └── index.html
```

---

# Tests

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

# RESOURCES

- [Understanding epoll](https://fraugsleeves.dev/blog/epoll)
- [MDN HTTP Cookies Guide](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Cookies)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Sockets and Network Programming in C](https://www.codequoi.com/en/sockets-and-network-programming-in-c/)
- [The Method to epoll’s Madness](https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642)


# Authors

* Malika Chaouki
* Zineb Elgharbaou
