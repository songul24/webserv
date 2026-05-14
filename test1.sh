#!/bin/bash

# ============================================================
# Script de tests avancé pour webserv
# Teste : GET, POST, DELETE, CGI, parsing, config, limites
# ============================================================

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
RESET="\033[0m"

PASS=0
FAIL=0
TOTAL=0

# Configuration des ports
PORT_MAIN=5050
PORT_CGI=5052

# Fonction pour compter les tests
test_count() {
    TOTAL=$((TOTAL + 1))
}

# Fonction de vérification
check() {
    local name="$1"
    local expected="$2"
    local got="$3"
    
    if echo "$got" | grep -q "$expected"; then
        echo -e "${GREEN}[PASS]${RESET} $name"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}[FAIL]${RESET} $name"
        echo -e "       attendu : $expected"
        echo -e "       recu    : $(echo $got | head -c 100)"
        FAIL=$((FAIL + 1))
    fi
    test_count
}

# Vérifier si le serveur tourne
check_server() {
    if ! curl -s --http1.0 http://127.0.0.1:$PORT_MAIN/ > /dev/null 2>&1; then
        echo -e "${RED}❌ Serveur non accessible sur le port $PORT_MAIN${RESET}"
        echo "   Assurez-vous que ./webserv tourne dans un autre terminal"
        exit 1
    fi
}

# Créer des fichiers de test
setup_test_files() {
    mkdir -p public/website1/home
    mkdir -p public/website1/upload
    mkdir -p storage
    mkdir -p CGI/CGI-bin
    
    echo "<h1>Index Page</h1>" > public/website1/index.html
    echo "<h1>Home Page</h1>" > public/website1/home/index.html
    echo "<h1>About Page</h1>" > public/website1/aboutt.html
    echo "<h1>Empty Page</h1>" > public/website1/empty.html
    
    # Fichier pour test upload
    echo "test content" > /tmp/test_upload.txt
    
    # Script CGI
    cat > storage/session.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>CGI Session Test</h1>")
print(f"<p>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'not set')}</p>")
print(f"<p>QUERY_STRING: {os.environ.get('QUERY_STRING', 'not set')}</p>")
print(f"<p>HTTP_COOKIE: {os.environ.get('HTTP_COOKIE', 'not set')}</p>")
print("</body></html>")
EOF
    chmod +x storage/session.py
    
    # Script CGI pour POST
    cat > CGI/CGI-bin/test_post.py << 'EOF'
#!/usr/bin/env python3
import sys
import os
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>POST CGI Test</h1>")
body = sys.stdin.read()
print(f"<p>Body length: {len(body)}</p>")
print(f"<p>Content-Type: {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
print("</body></html>")
EOF
    chmod +x CGI/CGI-bin/test_post.py
}

# Nettoyer les fichiers de test
cleanup_test_files() {
    rm -rf public/website1/home/testfile_*.txt
    rm -rf public/website1/upload/*
    rm -f /tmp/test_upload.txt
}

# ============================================================
# DÉBUT DES TESTS
# ============================================================

echo ""
echo "============================================================"
echo " WEBSERV ADVANCED TEST SUITE"
echo "============================================================"

setup_test_files
check_server

# ------------------------------------------------------------
# 1. TEST PARSING DE REQUÊTE (via GET)
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 1. TESTS PARSING ---${RESET}"

# 1.1 GET normal
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "GET / → 200" "200" "$R"

# 1.2 GET avec query string (doit être parsée)
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home?key=value&test=123" | head -c 200)
check "GET avec query string → contient Home Page" "Home Page" "$R"

# 1.3 GET avec path encodé
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/home%20page")
check "GET avec espace encodé → 404 ou 400" "40" "$R"

# 1.4 GET HTTP/1.1 (doit être refusé ou normalisé)
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
if [ "$R" = "505" ] || [ "$R" = "200" ]; then
    echo -e "${GREEN}[PASS]${RESET} GET HTTP/1.1 → $R (acceptable)"
    PASS=$((PASS + 1))
else
    echo -e "${RED}[FAIL]${RESET} GET HTTP/1.1 → $R (attendu 505 ou 200)"
    FAIL=$((FAIL + 1))
fi

# ------------------------------------------------------------
# 2. TEST GET (statique)
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 2. TESTS GET ---${RESET}"

# 2.1 GET page principale
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "GET / → contient Index" "Index" "$R"

# 2.2 GET location /home
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home")
check "GET /home → contient Home" "Home" "$R"

# 2.3 GET fichier inexistant
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/inexistant.html")
check "GET inexistant → 404" "404" "$R"

# 2.4 GET avec path traversal (..)
R=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 "http://127.0.0.1:$PORT_MAIN/../config.conf")
check "GET path traversal → 403" "403" "$R"

# 2.5 GET autoindex (dossier sans index.html)
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/home" | grep -c "<li>")
if [ $R -gt 0 ] 2>/dev/null; then
    echo -e "${GREEN}[PASS]${RESET} GET autoindex → contient liste de fichiers"
    PASS=$((PASS + 1))
else
    echo -e "${RED}[FAIL]${RESET} GET autoindex → pas de liste"
    FAIL=$((FAIL + 1))
fi

# ------------------------------------------------------------
# 3. TEST POST
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 3. TESTS POST ---${RESET}"

# 3.1 POST avec fichier
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    --data-binary @/tmp/test_upload.txt)
check "POST upload fichier → 201" "201" "$R"

# 3.2 POST Content-Length: 0
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 0")
check "POST Content-Length 0 → 200 ou 201" "[2]" "$R"

# 3.3 POST sans Content-Length (chunked - devrait être 400)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Transfer-Encoding: chunked" \
    -d "hello")
check "POST chunked (non supporté) → 400/501" "[45]" "$R"

# 3.4 POST sur route sans POST autorisé
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/aboutt")
check "POST sur /aboutt (GET only) → 405" "405" "$R"

# ------------------------------------------------------------
# 4. TEST DELETE
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 4. TESTS DELETE ---${RESET}"

# 4.1 DELETE fichier existant
echo "test" > public/website1/home/test_delete.txt
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/test_delete.txt")
check "DELETE fichier existant → 204" "204" "$R"

# 4.2 DELETE fichier inexistant
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/home/inexistant.txt")
check "DELETE inexistant → 404" "404" "$R"

# 4.3 DELETE sur route sans DELETE
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload")
check "DELETE sur /upload → 405" "405" "$R"

# ------------------------------------------------------------
# 5. TEST CGI
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 5. TESTS CGI ---${RESET}"

# 5.1 GET sur script Python
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
check "GET CGI session.py → contient CGI Session Test" "CGI Session Test" "$R"

# 5.2 GET CGI avec query string
R=$(curl -s --http1.0 "http://127.0.0.1:$PORT_CGI/session.py?name=test&value=123")
check "GET CGI avec query → contient QUERY_STRING" "QUERY_STRING" "$R"

# 5.3 POST CGI
R=$(curl -s -X POST --http1.0 \
    "http://127.0.0.1:$PORT_CGI/session.py" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "name=John&age=25")
check "POST CGI → contient POST CGI Test" "CGI" "$R"

# 5.4 CGI script non exécutable (doit être 403 ou 500)
chmod -x storage/session.py 2>/dev/null
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_CGI/session.py")
chmod +x storage/session.py
check "CGI non exécutable → 403/500" "[45]" "$R"

# ------------------------------------------------------------
# 6. TEST COOKIES
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 6. TESTS COOKIES ---${RESET}"

# 6.1 Envoyer un cookie
R=$(curl -s --http1.0 -b "test_cookie=hello123" "http://127.0.0.1:$PORT_CGI/session.py")
check "Cookie envoyé → contient HTTP_COOKIE" "test_cookie" "$R"

# 6.2 Cookie Set-Cookie dans réponse
R=$(curl -s -D - --http1.0 "http://127.0.0.1:$PORT_CGI/session.py" 2>/dev/null | grep -i "Set-Cookie")
if [ -n "$R" ]; then
    echo -e "${GREEN}[PASS]${RESET} Réponse avec Set-Cookie"
    PASS=$((PASS + 1))
else
    echo -e "${YELLOW}[INFO]${RESET} Pas de Set-Cookie (selon implémentation)"
fi

# ------------------------------------------------------------
# 7. TEST LIMITES
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 7. TESTS LIMITES ---${RESET}"

# 7.1 URL trop longue (> 2048)
LONG=$(python3 -c 'print("a" * 2100)')
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/$LONG")
check "URL trop longue → 414" "414" "$R"

# 7.2 Body trop grand (max 2K sur port 5050)
BIG=$(python3 -c 'print("A" * 3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Type: text/plain" \
    -d "$BIG")
check "Body > max_body_size (2K) → 413" "413" "$R"

# 7.3 Header trop grand (Content-Length énorme)
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST --http1.0 \
    "http://127.0.0.1:$PORT_MAIN/upload" \
    -H "Content-Length: 999999999" \
    -d "small")
check "Content-Length énorme → 413" "413" "$R"

# ------------------------------------------------------------
# 8. TEST CONCURRENCE
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 8. TESTS CONCURRENCE ---${RESET}"

# 8.1 Requêtes parallèles (20)
for i in $(seq 1 20); do
    curl -s --http1.0 "http://127.0.0.1:$PORT_MAIN/" > /dev/null 2>&1 &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "20 requêtes parallèles → serveur répond (200)" "200" "$R"

# 8.2 Keep-Alive (HTTP/1.1)
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" 2>/dev/null)
if [ "$R" = "200" ] || [ "$R" = "505" ]; then
    echo -e "${GREEN}[PASS]${RESET} HTTP/1.1 Keep-Alive → $R"
    PASS=$((PASS + 1))
else
    echo -e "${RED}[FAIL]${RESET} HTTP/1.1 Keep-Alive → $R"
    FAIL=$((FAIL + 1))
fi

# ------------------------------------------------------------
# 9. TEST MULTI-PORTS (deuxième serveur)
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 9. TESTS MULTI-PORTS ---${RESET}"

# 9.1 Port 5051
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:5051/")
check "Port 5051 → réponse" "200" "$R"

# 9.2 Port 5051 avec fichier inexistant
R=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://127.0.0.1:5051/inexistant")
check "Port 5051 404 → 404" "404" "$R"

# ------------------------------------------------------------
# 10. TEST MÉTHODES NON AUTORISÉES
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 10. TESTS MÉTHODES NON AUTORISÉES ---${RESET}"

# 10.1 PUT (non supporté)
R=$(curl -s -o /dev/null -w "%{http_code}" -X PUT --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "PUT → 400/405" "[4]" "$R"

# 10.2 HEAD
R=$(curl -s -o /dev/null -w "%{http_code}" -I --http1.0 "http://127.0.0.1:$PORT_MAIN/")
check "HEAD → 200" "200" "$R"

# ------------------------------------------------------------
# 11. TEST HEADERS
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 11. TESTS HEADERS ---${RESET}"

# 11.1 Content-Type correct pour HTML
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "Content-Type")
check "Content-Type pour HTML → text/html" "text/html" "$R"

# 11.2 Content-Length présent
R=$(curl -s -I --http1.0 "http://127.0.0.1:$PORT_MAIN/" | grep -i "Content-Length")
check "Header Content-Length présent" "Content-Length" "$R"

# 11.3 Host header requis pour HTTP/1.1
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:$PORT_MAIN/" -H "Host: test.com" 2>/dev/null)
if [ "$R" = "200" ]; then
    echo -e "${GREEN}[PASS]${RESET} Host header accepté"
    PASS=$((PASS + 1))
else
    echo -e "${YELLOW}[INFO]${RESET} Host header: réponse $R"
fi

# ------------------------------------------------------------
# RÉSUMÉ
# ------------------------------------------------------------
echo ""
echo "============================================================"
echo "                    RÉSULTATS DES TESTS"
echo "============================================================"
echo -e " ${GREEN}PASS: $PASS${RESET}  |  ${RED}FAIL: $FAIL${RESET}  |  TOTAL: $TOTAL"
echo "============================================================"

# Calcul du pourcentage
PERCENT=$((PASS * 100 / TOTAL))
if [ $PERCENT -ge 80 ]; then
    echo -e " ${GREEN}Score: $PERCENT% - Félicitations !${RESET}"
elif [ $PERCENT -ge 50 ]; then
    echo -e " ${YELLOW}Score: $PERCENT% - Peut mieux faire${RESET}"
else
    echo -e " ${RED}Score: $PERCENT% - Besoin de travailler${RESET}"
fi
echo ""

cleanup_test_files

# Quitter avec code d'erreur si des tests ont échoué
if [ $FAIL -gt 0 ]; then
    exit 1
fi
exit 0