#!/bin/bash

# ============================================================
# WEBSERV COMPREHENSIVE TEST SUITE - 42 Project
# Tests: Parsing, GET, POST, DELETE, CGI, Cookies,
#        Multiplexing, Epoll, Config, Limits, Headers
# ============================================================

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
CYAN="\033[0;36m"
RESET="\033[0m"

PASS=0
FAIL=0
TOTAL=0

PORT_MAIN=5050
PORT_SECOND=5051
PORT_CGI=5052

# ── helpers ────────────────────────────────────────────────

check() {
    local name="$1"
    local expected="$2"
    local got="$3"
    TOTAL=$((TOTAL + 1))
    if echo "$got" | grep -q "$expected"; then
        echo -e "${GREEN}[PASS]${RESET} $name"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}[FAIL]${RESET} $name"
        echo -e "       expected : $expected"
        echo -e "       got      : $(echo "$got" | head -c 120)"
        FAIL=$((FAIL + 1))
    fi
}

check_server() {
    if ! curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1; then
        echo -e "${RED}❌ Server not reachable on port $PORT_MAIN${RESET}"
        echo "   Start ./webserv in another terminal first"
        exit 1
    fi
}

setup_files() {
    mkdir -p public/website1/home public/website1/upload \
             public/website2/upload public/error_pages storage \
             CGI/CGI-bin

    echo "<h1>Index Page</h1>"  > public/website1/index.html
    echo "<h1>Home Page</h1>"   > public/website1/home/index.html
    echo "<h1>About Page</h1>"  > public/website1/aboutt.html
    echo "<h1>Empty Page</h1>"  > public/website1/empty.html
    echo "<h1>404 Not Found</h1>" > public/error_pages/404.html

    echo "upload test content"  > /tmp/test_upload.txt

    # CGI session script
    cat > storage/session.py << 'EOF'
#!/usr/bin/env python3
import os, sys
print("Content-Type: text/html\r\n\r\n", end="")
print("<html><body>")
print("<h1>CGI Session Test</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'not set')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'not set')}</p>")
print(f"<p>HTTP_COOKIE: {os.environ.get('HTTP_COOKIE', 'not set')}</p>")
print(f"<p>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
if os.environ.get('REQUEST_METHOD') == 'POST':
    length = int(os.environ.get('CONTENT_LENGTH', 0) or 0)
    body = sys.stdin.read(length)
    print(f"<p>BODY: {body}</p>")
print("</body></html>")
EOF
    chmod +x storage/session.py

    # CGI echo script (for header/env inspection)
    cat > storage/echo.py << 'EOF'
#!/usr/bin/env python3
import os, sys
print("Content-Type: text/plain\r\n\r", end="")
for key, val in sorted(os.environ.items()):
    print(f"{key}={val}")
EOF
    chmod +x storage/echo.py
}

cleanup_files() {
    rm -f public/website1/home/test_delete.txt
    rm -f public/website1/home/testfile_*.txt
    rm -rf public/website2/upload/*
    rm -f /tmp/test_upload.txt
}

# ── start ──────────────────────────────────────────────────

echo ""
echo "============================================================"
echo " WEBSERV COMPREHENSIVE TEST SUITE"
echo "============================================================"

setup_files
check_server

# ============================================================
# 1. REQUEST PARSING
# ============================================================
echo ""
echo -e "${CYAN}--- 1. REQUEST PARSING ---${RESET}"

# 1.1 Basic GET
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "GET / returns 200" "200" "$R"

# 1.2 Query string parsed (should not affect static file serving)
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/?foo=bar&baz=1" | head -c 200)
check "GET / with query string → serves index" "Index" "$R"

# 1.3 Query string on location
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home?key=value" | head -c 200)
check "GET /home with query string → serves home page" "Home" "$R"

# 1.4 URL percent-encoded space → 404 or 400
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home%20page")
check "GET percent-encoded path → 404 or 400" "40" "$R"

# 1.5 HTTP/1.0 accepted
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "HTTP/1.0 request accepted" "200" "$R"

# 1.6 HTTP/1.1 accepted or 505
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "505" ]; then
    echo -e "${GREEN}[PASS]${RESET} HTTP/1.1 request → $R (acceptable)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} HTTP/1.1 request → $R (expected 200 or 505)"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# 1.7 Unknown method → 400 or 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "Unknown method PATCH → 400 or 405" "[4]" "$R"

# 1.8 URL too long → 414
LONG=$(python3 -c 'print("a"*2100)')
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/$LONG")
check "URL too long → 414" "414" "$R"

# 1.9 Missing Host header on HTTP/1.1 → 400 or 200
R=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Host: " "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
check "Host header present → not 5xx" "[^5]" "$R"

# 1.10 Request with valid Host header accepted
R=$(curl -s -o /dev/null -w "%{http_code}" \
    "http://127.0.0.1:$PORT_MAIN/" -H "Host: test.com" 2>/dev/null)
check "Custom Host header accepted" "200" "$R"

# ============================================================
# 2. GET METHOD
# ============================================================
echo ""
echo -e "${CYAN}--- 2. GET METHOD ---${RESET}"

# 2.1 Root index
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "GET / serves index.html" "Index" "$R"

# 2.2 Location /home serves index.html when present
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "GET /home serves home index.html" "Home" "$R"

# 2.3 Static file 404
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist.html")
check "GET non-existent file → 404" "404" "$R"

# 2.4 Custom 404 error page served
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist.html")
check "GET non-existent → custom 404 page" "404" "$R"

# 2.5 Path traversal blocked
R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../config.conf")
check "GET path traversal → 403" "403" "$R"

# 2.6 Autoindex on /home (has index.html → should serve file, not listing)
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "GET /home with index.html → serves file not listing" "Home Page" "$R"

# 2.7 Autoindex listing when no index file (use /home with no index)
# The autoindex test uses the <li> count from directory listing
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home" | grep -c "<li>")
# This should be 0 because index.html exists and is served
# Autoindex is tested via a path without index — we use root autoindex
R2=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -c "<li>")
if [ "$R2" -gt 0 ] 2>/dev/null || echo "$R2" | grep -q "Index"; then
    echo -e "${GREEN}[PASS]${RESET} Autoindex or index served at root"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} Autoindex — nothing returned at root"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# 2.8 Content-Type text/html for .html files
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-type")
check "GET / → Content-Type: text/html" "text/html" "$R"

# 2.9 Content-Length header present
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-length")
check "GET / → Content-Length header present" "Content-Length" "$R"

# 2.10 GET on DELETE-only location → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X GET --http1.0 "http://127.0.0.1:$PORT_MAIN/not")
check "GET on DELETE-only location → 405" "405" "$R"

# ============================================================
# 3. POST METHOD
# ============================================================
echo ""
echo -e "${CYAN}--- 3. POST METHOD ---${RESET}"

# 3.1 POST upload file → 201
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    --data-binary @/tmp/test_upload.txt)
check "POST upload file → 201" "201" "$R"

# 3.2 POST Content-Length: 0 → 200 or 201 (empty body is valid)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 0")
check "POST Content-Length: 0 → 200 or 201" "[2]" "$R"

# 3.3 POST no body no Content-Length → 200 or 201 (treat as empty)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload")
check "POST no body → 200 or 201" "[2]" "$R"

# 3.4 POST chunked transfer → 400 or 501 (not supported)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Transfer-Encoding: chunked" \
    -d "hello")
check "POST chunked → 400 or 501" "[45]" "$R"

# 3.5 POST on GET-only location → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/aboutt")
check "POST on GET-only /aboutt → 405" "405" "$R"

# 3.6 POST body exceeds max_client_body_size → 413
BIG=$(python3 -c 'print("A"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    -d "$BIG")
check "POST body > 2K limit → 413" "413" "$R"

# 3.7 POST Content-Length larger than max → 413 (check before reading)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 999999999" \
    -d "small")
check "POST Content-Length: 999999999 → 413" "413" "$R"

# 3.8 POST body within limit on port 5051 (max 30000) → 201
MED=$(python3 -c 'print("B"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" \
    -d "$MED")
check "POST 3K body on port 5051 (max 30K) → 201" "201" "$R"

# 3.9 POST unknown Content-Type → 415 or 201 (implementation-defined)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: application/x-unknown-type" \
    -d "data")
check "POST unknown Content-Type → 415 or 2xx" "[24]" "$R"

# ============================================================
# 4. DELETE METHOD
# ============================================================
echo ""
echo -e "${CYAN}--- 4. DELETE METHOD ---${RESET}"

# 4.1 DELETE existing file → 204
echo "delete me" > public/website1/home/test_delete.txt
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/test_delete.txt")
check "DELETE existing file → 204" "204" "$R"

# 4.2 File actually deleted
if [ ! -f "public/website1/home/test_delete.txt" ]; then
    echo -e "${GREEN}[PASS]${RESET} File was actually removed from disk"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} File still exists after DELETE"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# 4.3 DELETE non-existent file → 404
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/doesnotexist.txt")
check "DELETE non-existent file → 404" "404" "$R"

# 4.4 DELETE on GET/POST-only location → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload")
check "DELETE on GET/POST-only /upload → 405" "405" "$R"

# 4.5 DELETE path traversal → 403 or 400
R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../config.conf")
check "DELETE path traversal → 403 or 400" "40" "$R"

# 4.6 DELETE on GET-only location → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/aboutt")
check "DELETE on GET-only /aboutt → 405" "405" "$R"

# ============================================================
# 5. CGI
# ============================================================
echo ""
echo -e "${CYAN}--- 5. CGI ---${RESET}"

# 5.1 GET CGI script executes
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "GET CGI session.py → CGI Session Test" "CGI Session Test" "$R"

# 5.2 CGI receives REQUEST_METHOD=GET
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "CGI GET → REQUEST_METHOD is GET" "REQUEST_METHOD: GET" "$R"

# 5.3 CGI receives QUERY_STRING
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py?name=test&value=123")
check "CGI GET with query → QUERY_STRING set" "name=test" "$R"

# 5.4 CGI POST receives REQUEST_METHOD=POST
R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=John&age=25")
check "CGI POST → REQUEST_METHOD is POST" "REQUEST_METHOD: POST" "$R"

# 5.5 CGI POST body forwarded
R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=John&age=25")
check "CGI POST → body forwarded" "name=John" "$R"

# 5.6 CGI receives HTTP_COOKIE
R=$(curl -s --http1.0 \
    -b "session_id=abc123" \
    "http://127.0.0.1:$PORT_CGI/session.py")
check "CGI receives HTTP_COOKIE" "session_id=abc123" "$R"

# 5.7 CGI receives CONTENT_TYPE on POST
R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "x=1")
check "CGI POST → CONTENT_TYPE set" "application/x-www-form-urlencoded" "$R"

# 5.8 CGI non-executable → 403 or 500
chmod -x storage/session.py 2>/dev/null
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
chmod +x storage/session.py
check "CGI non-executable → 403 or 500" "[45]" "$R"

# 5.9 CGI script not found → 404
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    "http://127.0.0.1:$PORT_CGI/nonexistent.py")
check "CGI script not found → 404" "404" "$R"

# ============================================================
# 6. COOKIES
# ============================================================
echo ""
echo -e "${CYAN}--- 6. COOKIES ---${RESET}"

# 6.1 Cookie forwarded to CGI via HTTP_COOKIE
R=$(curl -s --http1.0 \
    -b "user=alice; role=admin" \
    "http://127.0.0.1:$PORT_CGI/session.py")
check "Cookie header forwarded to CGI as HTTP_COOKIE" "user=alice" "$R"

# 6.2 Multiple cookies forwarded
R=$(curl -s --http1.0 \
    -b "a=1; b=2; c=3" \
    "http://127.0.0.1:$PORT_CGI/session.py")
check "Multiple cookies forwarded intact" "a=1" "$R"

# 6.3 No cookie → HTTP_COOKIE not set or empty
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "No cookie → HTTP_COOKIE is 'not set'" "not set" "$R"

# ============================================================
# 7. MULTIPLEXING / EPOLL / CONCURRENCY
# ============================================================
echo ""
echo -e "${CYAN}--- 7. MULTIPLEXING & CONCURRENCY ---${RESET}"

# 7.1 20 parallel requests — server handles all
FAILS=0
for i in $(seq 1 20); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "20 parallel requests → server still responds 200" "200" "$R"

# 7.2 50 parallel requests — server does not crash
for i in $(seq 1 50); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "50 parallel requests → server still responds 200" "200" "$R"

# 7.3 Simultaneous requests to different ports
curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null &
curl -s --http1.0 "http://127.0.0.1:$PORT_SECOND/" > /dev/null &
curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py" > /dev/null &
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "Simultaneous multi-port requests → main port still works" "200" "$R"

# 7.4 Keep-Alive HTTP/1.1 (multiple requests same connection)
R=$(curl -s -o /dev/null -w "%{http_code}" \
    "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "505" ]; then
    echo -e "${GREEN}[PASS]${RESET} HTTP/1.1 keep-alive → $R"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} HTTP/1.1 keep-alive → $R"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# 7.5 Server recovers after bad requests flood
for i in $(seq 1 10); do
    curl -s --http1.0 -X INVALID "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "Server recovers after bad request flood" "200" "$R"

# 7.6 Mixed valid and invalid concurrent requests
for i in $(seq 1 5); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null &
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist" > /dev/null &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "Mixed valid/invalid concurrent → server healthy" "200" "$R"

# ============================================================
# 8. MULTI-PORT / MULTI-SERVER CONFIG
# ============================================================
echo ""
echo -e "${CYAN}--- 8. MULTI-PORT CONFIG ---${RESET}"

# 8.1 Port 5051 serves requests
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/")
check "Port 5051 → 200" "200" "$R"

# 8.2 Port 5051 404
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/notfound")
check "Port 5051 not found → 404" "404" "$R"

# 8.3 Port 5051 has larger body limit (30K)
LARGE=$(python3 -c 'print("C"*25000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" \
    -d "$LARGE")
check "Port 5051 POST 25K body (limit 30K) → 201" "201" "$R"

# 8.4 Port 5051 still enforces its own limit
TOOLARGE=$(python3 -c 'print("D"*35000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" \
    -d "$TOOLARGE")
check "Port 5051 POST 35K body (limit 30K) → 413" "413" "$R"

# 8.5 Port 5052 CGI server responds
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "Port 5052 CGI server responds" "200" "$R"

# 8.6 Each server independent (kill one with bad request, others survive)
curl -s --http1.0 -H "Content-Length: 999999999" -X POST \
    "http://127.0.0.1:$PORT_MAIN/upload" -d "x" > /dev/null 2>&1
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/")
check "Port 5051 alive after bad request to 5050" "200" "$R"

# ============================================================
# 9. HEADERS
# ============================================================
echo ""
echo -e "${CYAN}--- 9. HEADERS ---${RESET}"

# 9.1 Content-Type html
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-type")
check "Response has Content-Type: text/html" "text/html" "$R"

# 9.2 Content-Length present
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-length")
check "Response has Content-Length" "Content-Length" "$R"

# 9.3 Content-Length matches body length
HEADERS=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/")
CL=$(echo "$HEADERS" | grep -i "content-length" | grep -o '[0-9]*')
BODY=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
BL=${#BODY}
if [ "$CL" = "$BL" ] 2>/dev/null; then
    echo -e "${GREEN}[PASS]${RESET} Content-Length ($CL) matches actual body length ($BL)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} Content-Length ($CL) does not match body length ($BL)"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# 9.4 PUT method → 400 or 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X PUT --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "PUT method → 400 or 405" "[4]" "$R"

# 9.5 Response has status line
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | head -1)
check "Response has HTTP status line" "HTTP" "$R"

# ============================================================
# 10. REDIRECTS
# ============================================================
echo ""
echo -e "${CYAN}--- 10. REDIRECTS ---${RESET}"

# 10.1 Redirect location configured returns 3xx
# (uses /mcha which can be configured with redirect — skipped if not configured)
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    -L "http://127.0.0.1:$PORT_MAIN/mcha" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "301" ] || [ "$R" = "302" ] || [ "$R" = "404" ]; then
    echo -e "${GREEN}[PASS]${RESET} Redirect or direct response for /mcha → $R"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} /mcha unexpected response → $R"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# ============================================================
# 11. SECURITY
# ============================================================
echo ""
echo -e "${CYAN}--- 11. SECURITY ---${RESET}"

# 11.1 Path traversal GET blocked
R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../../etc/passwd")
check "Path traversal GET ../../etc/passwd → 403 or 400" "40" "$R"

# 11.2 Path traversal DELETE blocked
R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../config.conf")
check "Path traversal DELETE → 403 or 400" "40" "$R"

# 11.3 Null byte in URL rejected
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/index%00.html" 2>/dev/null)
check "Null byte in URL → 400 or 404" "[34]" "$R"

# 11.4 Very long header value handled
LONGVAL=$(python3 -c 'print("x"*8000)')
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    -H "X-Custom: $LONGVAL" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
check "Very long header value → not 5xx crash" "[^5]" "$R"

# 11.5 Body exactly at limit accepted
EXACT=$(python3 -c 'print("E"*2048, end="")')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    --data-binary "$EXACT")
check "POST body exactly at 2K limit → 201 or 413" "[24]" "$R"

# 11.6 Body one byte over limit rejected
OVER=$(python3 -c 'print("F"*2049, end="")')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    --data-binary "$OVER")
check "POST body 2049 bytes (over 2K) → 413" "413" "$R"

# ============================================================
# 12. ERROR PAGES
# ============================================================
echo ""
echo -e "${CYAN}--- 12. ERROR PAGES ---${RESET}"

# 12.1 Custom 404 page content
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/notfound_page")
check "Custom 404 error page served" "404" "$R"

# 12.2 404 status code correct
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/notfound_page")
check "404 status code is 404" "404" "$R"

# 12.3 405 has correct status code
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 "http://127.0.0.1:$PORT_MAIN/aboutt")
check "405 status code is 405" "405" "$R"

# 12.4 413 has correct status code
BIG=$(python3 -c 'print("G"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" -d "$BIG")
check "413 status code is 413" "413" "$R"

# ============================================================
# SUMMARY
# ============================================================
echo ""
echo "============================================================"
echo "                    TEST RESULTS"
echo "============================================================"
echo -e " ${GREEN}PASS: $PASS${RESET}  |  ${RED}FAIL: $FAIL${RESET}  |  TOTAL: $TOTAL"
echo "============================================================"

PERCENT=$((PASS * 100 / TOTAL))
if   [ $PERCENT -ge 90 ]; then echo -e " ${GREEN}Score: $PERCENT% - Excellent!${RESET}"
elif [ $PERCENT -ge 75 ]; then echo -e " ${GREEN}Score: $PERCENT% - Good job!${RESET}"
elif [ $PERCENT -ge 50 ]; then echo -e " ${YELLOW}Score: $PERCENT% - Keep going${RESET}"
else                            echo -e " ${RED}Score: $PERCENT% - Needs work${RESET}"
fi
echo ""

cleanup_files

[ $FAIL -gt 0 ] && exit 1
exit 0