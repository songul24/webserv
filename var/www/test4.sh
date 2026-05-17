#!/bin/bash

# ============================================================
# WEBSERV COMPLETE TEST SUITE - 42 Project
# Mandatory: Parsing, GET, POST, DELETE, CGI, Headers,
#            Multi-port, Security, Limits, Error pages
# Bonus:     Cookies, Sessions, Multi-CGI types
# ============================================================

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
CYAN="\033[0;36m"
MAGENTA="\033[0;35m"
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
        echo -e "       got      : $(echo "$got" | head -c 200)"
        FAIL=$((FAIL + 1))
    fi
}

check_not() {
    local name="$1"
    local unexpected="$2"
    local got="$3"
    TOTAL=$((TOTAL + 1))
    if echo "$got" | grep -q "$unexpected"; then
        echo -e "${RED}[FAIL]${RESET} $name"
        echo -e "       should NOT contain : $unexpected"
        echo -e "       got               : $(echo "$got" | head -c 200)"
        FAIL=$((FAIL + 1))
    else
        echo -e "${GREEN}[PASS]${RESET} $name"
        PASS=$((PASS + 1))
    fi
}

check_server() {
    if ! curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1; then
        echo -e "${RED}❌ Server not reachable on port $PORT_MAIN${RESET}"
        echo "   Start ./webserv config4.conf in another terminal first"
        exit 1
    fi
    if ! curl -s --http1.0 "http://127.0.0.1:$PORT_SECOND/" > /dev/null 2>&1; then
        echo -e "${RED}❌ Server not reachable on port $PORT_SECOND${RESET}"
        exit 1
    fi
    if ! curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/" > /dev/null 2>&1; then
        echo -e "${RED}❌ Server not reachable on port $PORT_CGI${RESET}"
        exit 1
    fi
}

setup_files() {
    mkdir -p public/website1/home public/website1/sub/deep \
             public/website2/upload public/error_pages \
             storage CGI/CGI-bin

    printf "<h1>Index Page</h1>"   > public/website1/index.html
    printf "<h1>Home Page</h1>"    > public/website1/home/index.html
    printf "<h1>About Page</h1>"   > public/website1/aboutt.html
    printf "<h1>Empty Page</h1>"   > public/website1/empty.html
    printf "<h1>Sub Page</h1>"     > public/website1/sub/index.html
    printf "<h1>Deep Page</h1>"    > public/website1/sub/deep/index.html
    printf "<h1>404 Not Found</h1>" > public/error_pages/404.html
    printf "<h1>405 Not Allowed</h1>" > public/error_pages/405.html
    printf "<h1>413 Too Large</h1>"   > public/error_pages/413.html
    printf "upload test content"   > /tmp/test_upload.txt
    printf "binary\x00data\x01test" > /tmp/test_binary.bin

    # Main CGI session script — proper \r\n\r\n separator
    cat > storage/session.py << 'EOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/html\r\n\r\n")
sys.stdout.flush()
print("<html><body>")
print("<h1>CGI Session Test</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'not set')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'not set')}</p>")
print(f"<p>HTTP_COOKIE: {os.environ.get('HTTP_COOKIE', 'not set')}</p>")
print(f"<p>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
print(f"<p>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', 'not set')}</p>")
print(f"<p>SERVER_NAME: {os.environ.get('SERVER_NAME', 'not set')}</p>")
print(f"<p>SERVER_PORT: {os.environ.get('SERVER_PORT', 'not set')}</p>")
print(f"<p>PATH_INFO: {os.environ.get('PATH_INFO', 'not set')}</p>")
print(f"<p>SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'not set')}</p>")
if os.environ.get('REQUEST_METHOD') == 'POST':
    length = int(os.environ.get('CONTENT_LENGTH', 0) or 0)
    body = sys.stdin.read(length)
    print(f"<p>BODY: {body}</p>")
print("</body></html>")
EOF
    chmod +x storage/session.py

    # CGI env dump script
    cat > storage/env.py << 'EOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()
for key, val in sorted(os.environ.items()):
    print(f"{key}={val}")
EOF
    chmod +x storage/env.py

    # CGI cookie setter script
    cat > storage/setcookie.py << 'EOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("Set-Cookie: session_id=abc123; Path=/\r\n")
sys.stdout.write("Set-Cookie: user=testuser; Path=/\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()
print("<html><body><h1>Cookie Set</h1>")
print(f"<p>Existing cookies: {os.environ.get('HTTP_COOKIE', 'none')}</p>")
print("</body></html>")
EOF
    chmod +x storage/setcookie.py

    # CGI that reads POST body and echoes it
    cat > storage/echo_body.py << 'EOF'
#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()
length = int(os.environ.get('CONTENT_LENGTH', 0) or 0)
if length > 0:
    body = sys.stdin.read(length)
    sys.stdout.write(body)
else:
    sys.stdout.write("no body")
EOF
    chmod +x storage/echo_body.py

    # CGI that outputs large response
    cat > storage/large.py << 'EOF'
#!/usr/bin/env python3
import sys
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()
sys.stdout.write("X" * 50000)
EOF
    chmod +x storage/large.py

    # CGI that sleeps (timeout test)
    cat > storage/slow.py << 'EOF'
#!/usr/bin/env python3
import sys, time
time.sleep(15)
sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.write("late response")
EOF
    chmod +x storage/slow.py

    # CGI with exit error
    cat > storage/error.py << 'EOF'
#!/usr/bin/env python3
import sys
sys.exit(1)
EOF
    chmod +x storage/error.py
}

cleanup_files() {
    rm -f public/website1/home/test_delete.txt
    rm -f public/website1/home/testfile_*.txt
    rm -f public/website2/upload/*
    rm -f /tmp/test_upload.txt
    rm -f /tmp/test_binary.bin
    rm -f storage/body_*
}

section() {
    echo ""
    echo -e "${CYAN}--- $1 ---${RESET}"
}

# ── start ──────────────────────────────────────────────────

echo ""
echo "============================================================"
echo " WEBSERV COMPLETE TEST SUITE - 42 Project"
echo "============================================================"

setup_files
check_server

# ============================================================
# 1. REQUEST LINE PARSING
# ============================================================
section "1. REQUEST LINE PARSING"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "1.01 GET / returns 200" "200" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/?foo=bar&baz=1" | head -c 200)
check "1.02 GET with query string serves index" "Index" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home?key=value" | head -c 200)
check "1.03 GET location with query string" "Home" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home%20page")
check "1.04 Percent-encoded space path → 400 or 404" "40" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "1.05 HTTP/1.0 accepted" "200" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "505" ]; then
    echo -e "${GREEN}[PASS]${RESET} 1.06 HTTP/1.1 → $R (acceptable)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} 1.06 HTTP/1.1 → $R (expected 200 or 505)"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

R=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "1.07 Unknown method PATCH → 400 or 405" "[4]" "$R"

LONG=$(python3 -c 'print("a"*2100)')
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/$LONG")
check "1.08 URL too long → 414" "414" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" -H "Host: test.com" 2>/dev/null)
check "1.09 Custom Host header accepted" "200" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/index%00.html" 2>/dev/null)
check "1.10 Null byte in URL → 400 or 404" "[34]" "$R"

# ============================================================
# 2. GET METHOD
# ============================================================
section "2. GET METHOD"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "2.01 GET / serves index.html" "Index" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "2.02 GET /home serves home/index.html" "Home" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/aboutt")
check "2.03 GET /aboutt serves aboutt.html" "About" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist.html")
check "2.04 GET non-existent → 404" "404" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist.html")
check "2.05 GET non-existent → custom 404 page" "404" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 "http://127.0.0.1:$PORT_MAIN/../confi.conf")
check "2.06 Path traversal → 403" "403" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 "http://127.0.0.1:$PORT_MAIN/../../etc/passwd")
check "2.07 Deep path traversal → 403 or 400" "40" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "2.08 Index file takes priority over autoindex" "Home Page" "$R"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-type")
check "2.09 GET .html → Content-Type: text/html" "text/html" "$R"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-length")
check "2.10 GET → Content-Length header present" "Content-Length" "$R"

HEADERS=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/")
CL=$(echo "$HEADERS" | grep -i "content-length" | grep -o '[0-9]*')
BODY=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
BL=${#BODY}
if [ "$CL" = "$BL" ] 2>/dev/null; then
    echo -e "${GREEN}[PASS]${RESET} 2.11 Content-Length ($CL) matches body ($BL)"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} 2.11 Content-Length ($CL) does not match body ($BL)"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

R=$(curl -s -o /dev/null -w "%{http_code}" -X GET --http1.0 "http://127.0.0.1:$PORT_MAIN/not")
check "2.12 GET on DELETE-only location → 405" "405" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X PUT --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "2.13 PUT method → 400 or 405" "[4]" "$R"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | head -1)
check "2.14 Response has HTTP status line" "HTTP" "$R"

# Nested path
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/sub")
check "2.15 GET nested /sub serves sub/index.html" "Sub" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/sub/deep")
check "2.16 GET deeply nested /sub/deep serves file" "Deep" "$R"

# ============================================================
# 3. POST METHOD
# ============================================================
section "3. POST METHOD"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    --data-binary @/tmp/test_upload.txt)
check "3.01 POST upload file → 201" "201" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 0")
check "3.02 POST Content-Length: 0 → 2xx" "[2]" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload")
check "3.03 POST no body no Content-Length → 2xx" "[2]" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Transfer-Encoding: chunked" -d "hello")
check "3.04 POST chunked → 400 or 501" "[45]" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/aboutt")
check "3.05 POST on GET-only /aboutt → 405" "405" "$R"

BIG=$(python3 -c 'print("A"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" -d "$BIG")
check "3.06 POST body > 2K limit → 413" "413" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 999999999" -d "small")
check "3.07 POST Content-Length: 999999999 → 413" "413" "$R"

MED=$(python3 -c 'print("B"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" -d "$MED")
check "3.08 POST 3K on port 5051 (limit 30K) → 201" "201" "$R"

LARGE=$(python3 -c 'print("C"*25000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" -d "$LARGE")
check "3.09 POST 25K on port 5051 (limit 30K) → 201" "201" "$R"

TOOLARGE=$(python3 -c 'print("D"*35000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_SECOND/upload" \
    -H "Content-Type: text/plain" -d "$TOOLARGE")
check "3.10 POST 35K on port 5051 (limit 30K) → 413" "413" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: application/x-unknown-type" -d "data")
check "3.11 POST unknown Content-Type → 415 or 2xx" "[24]" "$R"

# Verify file actually saved
FNAME=$(ls public/website2/upload/ 2>/dev/null | head -1)
if [ -n "$FNAME" ]; then
    echo -e "${GREEN}[PASS]${RESET} 3.12 Uploaded file exists on disk"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} 3.12 No uploaded file found on disk"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# ============================================================
# 4. DELETE METHOD
# ============================================================
section "4. DELETE METHOD"

echo "delete me" > public/website1/home/test_delete.txt
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/test_delete.txt")
check "4.01 DELETE existing file → 204" "204" "$R"

if [ ! -f "public/website1/home/test_delete.txt" ]; then
    echo -e "${GREEN}[PASS]${RESET} 4.02 File actually removed from disk"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} 4.02 File still exists after DELETE"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/doesnotexist.txt")
check "4.03 DELETE non-existent → 404" "404" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload")
check "4.04 DELETE on GET/POST-only → 405" "405" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../config4.conf")
check "4.05 DELETE path traversal → 403 or 400" "40" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/aboutt")
check "4.06 DELETE on GET-only → 405" "405" "$R"

# DELETE then GET confirms gone
echo "temp" > public/website1/home/testfile_del2.txt
curl -s -X DELETE --http1.0 "http://127.0.0.1:$PORT_MAIN/home/testfile_del2.txt" > /dev/null
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home/testfile_del2.txt")
check "4.07 GET after DELETE → 404" "404" "$R"

# ============================================================
# 5. CGI — MANDATORY
# ============================================================
section "5. CGI — MANDATORY"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "5.01 GET CGI executes → CGI Session Test" "CGI Session Test" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "5.02 CGI GET → REQUEST_METHOD=GET" "REQUEST_METHOD: GET" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py?name=test&value=123")
check "5.03 CGI GET query → QUERY_STRING set" "name=test" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py?name=test&value=123")
check "5.04 CGI GET full query string preserved" "value=123" "$R"

R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=John&age=25")
check "5.05 CGI POST → REQUEST_METHOD=POST" "REQUEST_METHOD: POST" "$R"

R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=John&age=25")
check "5.06 CGI POST body forwarded" "name=John" "$R"

R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "x=1")
check "5.07 CGI POST → CONTENT_TYPE set" "application/x-www-form-urlencoded" "$R"

R=$(curl -s --http1.0 -b "session_id=abc123" "http://127.0.0.1:$PORT_CGI/session.py")
check "5.08 CGI receives HTTP_COOKIE" "session_id=abc123" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "5.09 CGI no cookie → HTTP_COOKIE not set" "not set" "$R"

chmod -x storage/session.py 2>/dev/null
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
chmod +x storage/session.py
check "5.10 CGI non-executable → 403 or 500" "[45]" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/nonexistent.py")
check "5.11 CGI not found → 404" "404" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py")
check "5.12 CGI env dump → SERVER_PORT set" "SERVER_PORT" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py")
check "5.13 CGI env → SERVER_NAME set" "SERVER_NAME" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py")
check "5.14 CGI env → GATEWAY_INTERFACE set" "GATEWAY_INTERFACE" "$R"

# CGI large output
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/large.py")
check "5.15 CGI large output (50K) → 200" "200" "$R"

# CGI timeout
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 12 "http://127.0.0.1:$PORT_CGI/slow.py")
check "5.16 CGI timeout → 504" "504" "$R"

# CGI bad exit code
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/error.py")
check "5.17 CGI exit(1) → 502" "502" "$R"

# ============================================================
# 6. CGI — ENVIRONMENT VARIABLES
# ============================================================
section "6. CGI — ENVIRONMENT VARIABLES"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py")
check "6.01 CGI env → REQUEST_METHOD present" "REQUEST_METHOD=GET" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py?a=1")
check "6.02 CGI env → QUERY_STRING present" "QUERY_STRING=a=1" "$R"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/env.py")
check "6.03 CGI env → SERVER_PORT=5052" "SERVER_PORT=5052" "$R"

R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/env.py" \
    -H "Content-Type: text/plain" \
    -d "hello")
check "6.04 CGI POST env → CONTENT_LENGTH set" "CONTENT_LENGTH=5" "$R"

R=$(curl -s --http1.0 -b "x=1" "http://127.0.0.1:$PORT_CGI/env.py")
check "6.05 CGI env → HTTP_COOKIE=x=1" "HTTP_COOKIE=x=1" "$R"

# ============================================================
# 7. COOKIES & SESSION (BONUS)
# ============================================================
section "7. COOKIES & SESSION (BONUS)"

R=$(curl -s --http1.0 -b "user=alice; role=admin" "http://127.0.0.1:$PORT_CGI/session.py")
check "7.01 Multiple cookies forwarded" "user=alice" "$R"

R=$(curl -s --http1.0 -b "a=1; b=2; c=3" "http://127.0.0.1:$PORT_CGI/session.py")
check "7.02 All cookie values preserved" "a=1" "$R"

# Set-Cookie in CGI response
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_CGI/setcookie.py" | grep -i "set-cookie")
check "7.03 CGI can set cookies via Set-Cookie header" "Set-Cookie\|set-cookie" "$R"

# Session: send cookie back, CGI sees it
R=$(curl -s --http1.0 -b "session_id=abc123" "http://127.0.0.1:$PORT_CGI/setcookie.py")
check "7.04 Session: existing cookie visible to CGI" "session_id=abc123" "$R"

# Cookie jar round-trip
COOKIEJAR=$(mktemp)
curl -s --http1.0 -c "$COOKIEJAR" "http://127.0.0.1:$PORT_CGI/setcookie.py" > /dev/null
R=$(curl -s --http1.0 -b "$COOKIEJAR" "http://127.0.0.1:$PORT_CGI/session.py")
rm -f "$COOKIEJAR"
check "7.05 Cookie jar round-trip — cookie sent back" "session_id=abc123" "$R"

# ============================================================
# 8. MULTI-PORT / MULTI-SERVER CONFIG
# ============================================================
section "8. MULTI-PORT CONFIG"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/")
check "8.01 Port 5051 → 200" "200" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/notfound")
check "8.02 Port 5051 not found → 404" "404" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "8.03 Port 5052 CGI server → 200" "200" "$R"

# Each port enforces its own body limit
curl -s --http1.0 -H "Content-Length: 999999999" -X POST \
    "http://127.0.0.1:$PORT_MAIN/upload" -d "x" > /dev/null 2>&1
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_SECOND/")
check "8.04 Port 5051 alive after bad request to 5050" "200" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "8.05 Port 5050 still responds after multi-port use" "200" "$R"

# Different roots per server
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "8.06 Port 5050 root serves website1 index" "Index" "$R"

# ============================================================
# 9. MULTIPLEXING & CONCURRENCY
# ============================================================
section "9. MULTIPLEXING & CONCURRENCY"

for i in $(seq 1 20); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "9.01 20 parallel requests → server healthy" "200" "$R"

for i in $(seq 1 50); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "9.02 50 parallel requests → server healthy" "200" "$R"

curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null &
curl -s --http1.0 "http://127.0.0.1:$PORT_SECOND/" > /dev/null &
curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py" > /dev/null &
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "9.03 Simultaneous multi-port requests → all healthy" "200" "$R"

for i in $(seq 1 10); do
    curl -s --http1.0 -X INVALID "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "9.04 Bad request flood → server recovers" "200" "$R"

for i in $(seq 1 5); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null &
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist" > /dev/null &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "9.05 Mixed valid/invalid concurrent → healthy" "200" "$R"

# Rapid sequential requests
for i in $(seq 1 30); do
    R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
    if [ "$R" != "200" ]; then
        echo -e "${RED}[FAIL]${RESET} 9.06 Sequential request $i failed with $R"
        FAIL=$((FAIL+1))
        TOTAL=$((TOTAL+1))
        break
    fi
done
if [ "$R" = "200" ]; then
    echo -e "${GREEN}[PASS]${RESET} 9.06 30 sequential requests all → 200"
    PASS=$((PASS+1))
    TOTAL=$((TOTAL+1))
fi

# ============================================================
# 10. HEADERS
# ============================================================
section "10. HEADERS"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-type")
check "10.01 Response has Content-Type: text/html" "text/html" "$R"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "content-length")
check "10.02 Response has Content-Length" "Content-Length\|content-length" "$R"

R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | head -1)
check "10.03 Response status line starts with HTTP" "HTTP" "$R"

# Connection: close on HTTP/1.0
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "connection")
check "10.04 HTTP/1.0 response has Connection header" "Connection\|connection" "$R"

# Very long header value — server must not crash
LONGVAL=$(python3 -c 'print("x"*8000)')
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    -H "X-Custom: $LONGVAL" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
check "10.05 Very long header value → not 5xx crash" "[^5]" "$R"

# Many headers
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    -H "X-A: 1" -H "X-B: 2" -H "X-C: 3" -H "X-D: 4" -H "X-E: 5" \
    "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
check "10.06 Many custom headers → 200" "200" "$R"

# ============================================================
# 11. SECURITY
# ============================================================
section "11. SECURITY"

R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../../etc/passwd")
check "11.01 Path traversal GET → 403 or 400" "40" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/../config4.conf")
check "11.02 Path traversal DELETE → 403 or 400" "40" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/index%00.html" 2>/dev/null)
check "11.03 Null byte in URL → 400 or 404" "[34]" "$R"

# Double encoding
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/%2e%2e/%2e%2e/etc/passwd")
check "11.04 URL encoded traversal → 403 or 400" "40" "$R"

EXACT=$(python3 -c 'print("E"*2048, end="")')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" --data-binary "$EXACT")
check "11.05 POST body exactly at 2K limit → 2xx or 413" "[24]" "$R"

OVER=$(python3 -c 'print("F"*2049, end="")')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" --data-binary "$OVER")
check "11.06 POST body 1 byte over limit → 413" "413" "$R"

# Response must not expose server internals
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/doesnotexist")
check_not "11.07 404 response does not expose stack trace" "Traceback\|Exception\|stack" "$R"

# ============================================================
# 12. ERROR PAGES
# ============================================================
section "12. ERROR PAGES"

R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/notfound_page")
check "12.01 Custom 404 page content served" "404" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/notfound_page")
check "12.02 404 status code correct" "404" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 "http://127.0.0.1:$PORT_MAIN/aboutt")
check "12.03 405 status code correct" "405" "$R"

BIG=$(python3 -c 'print("G"*3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" -d "$BIG")
check "12.04 413 status code correct" "413" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "12.05 400 or 405 for unsupported method" "[4]" "$R"

# 5xx — ensure default page returned not empty body
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/error.py")
check_not "12.06 502 response has a body" "^$" "$R"

# ============================================================
# 13. REDIRECTS
# ============================================================
section "13. REDIRECTS"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    -L "http://127.0.0.1:$PORT_MAIN/mcha" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "301" ] || [ "$R" = "302" ] || [ "$R" = "404" ]; then
    echo -e "${GREEN}[PASS]${RESET} 13.01 /mcha redirect or response → $R"
    PASS=$((PASS+1))
else
    echo -e "${RED}[FAIL]${RESET} 13.01 /mcha → $R (expected 2xx/3xx/404)"
    FAIL=$((FAIL+1))
fi
TOTAL=$((TOTAL+1))

# Redirect with Location header
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/mcha" 2>/dev/null | head -1)
if echo "$R" | grep -q "30[12]"; then
    LOCATION=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/mcha" | grep -i "^location:")
    if [ -n "$LOCATION" ]; then
        echo -e "${GREEN}[PASS]${RESET} 13.02 3xx redirect has Location header"
        PASS=$((PASS+1))
    else
        echo -e "${RED}[FAIL]${RESET} 13.02 3xx redirect missing Location header"
        FAIL=$((FAIL+1))
    fi
else
    echo -e "${GREEN}[PASS]${RESET} 13.02 /mcha not configured as redirect (acceptable)"
    PASS=$((PASS+1))
fi
TOTAL=$((TOTAL+1))

# ============================================================
# 14. STATIC FILE SERVING
# ============================================================
section "14. STATIC FILE SERVING"

R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "14.01 Root index.html served" "200" "$R"

# Trailing slash on directory
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home/")
check "14.02 Trailing slash on directory → 200" "200" "$R"

# No trailing slash — server redirects or serves
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "14.03 No trailing slash on directory → 200 or 301" "[23]" "$R"

# File with spaces (percent-encoded)
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/doesnotexist%20file.html")
check "14.04 Percent-encoded filename → 400 or 404" "40" "$R"

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
if   [ $PERCENT -ge 95 ]; then echo -e " ${GREEN}Score: $PERCENT% — Excellent! Ready for evaluation.${RESET}"
elif [ $PERCENT -ge 85 ]; then echo -e " ${GREEN}Score: $PERCENT% — Very good job!${RESET}"
elif [ $PERCENT -ge 75 ]; then echo -e " ${YELLOW}Score: $PERCENT% — Good, a few things to fix.${RESET}"
elif [ $PERCENT -ge 50 ]; then echo -e " ${YELLOW}Score: $PERCENT% — Keep going.${RESET}"
else                            echo -e " ${RED}Score: $PERCENT% — Needs significant work.${RESET}"
fi
echo ""

cleanup_files
[ $FAIL -gt 0 ] && exit 1
exit 0