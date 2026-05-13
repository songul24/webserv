#!/bin/bash

# ============================================================
# Script de tests pour webserv
# Usage : ./test.sh
# Prérequis : ./webserv config.conf doit tourner dans un autre terminal
# ============================================================

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
RESET="\033[0m"

PASS=0
FAIL=0

check()
{
    local name="$1"
    local expected="$2"
    local got="$3"

    if echo "$got" | grep -q "$expected"; then
        echo -e "${GREEN}[PASS]${RESET} $name"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}[FAIL]${RESET} $name"
        echo -e "       attendu : $expected"
        echo -e "       recu    : $(echo $got | head -c 80)"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo "============================================================"
echo " WEBSERV TEST SUITE"
echo "============================================================"

# ------------------------------------------------------------
# 1. GET
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 1. GET ---${RESET}"

# 1.1 GET page principale → 200
R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5050/)
check "GET / → 200" "200" "$R"

# 1.2 GET location /home → 200
R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5050/home)
check "GET /home → 200" "200" "$R"

# 1.3 GET fichier inexistant → 404
R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5050/fichier_qui_nexiste_pas.html)
check "GET fichier inexistant → 404" "404" "$R"

# 1.4 GET avec query string → 200 (query ignorée, fichier servi)
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:5050/home?key=value")
check "GET avec query string → 200" "200" "$R"

# 1.5 Autoindex activé → 200 avec liste de fichiers
R=$(curl -s http://127.0.0.1:5050/)
check "Autoindex → contient <html>" "<html" "$R"

# ------------------------------------------------------------
# 2. POST
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 2. POST ---${RESET}"

# 2.1 POST upload fichier texte → 201
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5050/upload \
    -H "Content-Type: text/plain" \
    -d "hello world")
check "POST upload texte → 201" "201" "$R"

# 2.2 POST body trop grand (> 2K sur port 5050) → 413
BIG=$(python3 -c 'print("A" * 3000)')
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5050/upload \
    -H "Content-Type: text/plain" \
    -d "$BIG")
check "POST body > max_body_size → 413" "413" "$R"

# 2.3 POST body trop grand mais OK sur port 5051 (max=30000) → 201
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5051/upload \
    -H "Content-Type: text/plain" \
    -d "$BIG")
check "POST body 3000 sur port 5051 (max=30000) → 201" "201" "$R"

# 2.4 POST Content-Type non supporté → 415
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5050/upload \
    -H "Content-Type: application/xml" \
    -d "<data>test</data>")
check "POST Content-Type inconnu → 415" "415" "$R"

# 2.5 POST sur route sans POST autorisé → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5050/aboutt \
    -H "Content-Type: text/plain" \
    -d "test")
check "POST sur /aboutt (GET only) → 405" "405" "$R"

# ------------------------------------------------------------
# 3. DELETE
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 3. DELETE ---${RESET}"

# 3.1 DELETE fichier existant → 204
echo "fichier test" > /tmp/testfile_delete.txt
cp /tmp/testfile_delete.txt ./public/website1/home/testfile_delete.txt
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    http://127.0.0.1:5050/home/testfile_delete.txt)
check "DELETE fichier existant → 204" "204" "$R"

# 3.2 DELETE fichier inexistant → 404
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    http://127.0.0.1:5050/home/fichier_inexistant_xyz.txt)
check "DELETE fichier inexistant → 404" "404" "$R"

# 3.3 DELETE sur route sans DELETE → 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    http://127.0.0.1:5050/upload)
check "DELETE sur /upload (GET POST only) → 405" "405" "$R"

# 3.4 DELETE path traversal → 403
R=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE \
    "http://127.0.0.1:5050/../etc/passwd")
check "DELETE path traversal → 403 ou 400" "40" "$R"

# ------------------------------------------------------------
# 4. Requêtes malformées
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 4. Requêtes malformées ---${RESET}"

# 4.1 Méthode inconnue → 400 ou 405
R=$(curl -s -o /dev/null -w "%{http_code}" -X PATCH http://127.0.0.1:5050/)
check "Méthode PATCH inconnue → 400/405" "40" "$R"

# 4.2 Path trop long → 414
LONGPATH=$(python3 -c 'print("a" * 2100)')
R=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:5050/$LONGPATH")
check "Path trop long → 414" "414" "$R"

# 4.3 POST sans body (Content-Length: 0) → 200 ou 201
R=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:5050/home \
    -H "Content-Length: 0")
check "POST Content-Length 0 → pas de crash (2xx ou 4xx)" "[24]" "$R"

# ------------------------------------------------------------
# 5. Connexions simultanées
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 5. Connexions simultanées ---${RESET}"

for i in $(seq 1 10); do
    curl -s -o /dev/null http://127.0.0.1:5050/ &
done
wait
R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5050/)
check "Serveur répond après 10 connexions parallèles → 200" "200" "$R"

# ------------------------------------------------------------
# 6. Deuxième serveur port 5051
# ------------------------------------------------------------
echo ""
echo -e "${YELLOW}--- 6. Port 5051 ---${RESET}"

R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5051/)
check "GET port 5051 → 200" "200" "$R"

R=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5051/inexistant)
check "GET inexistant port 5051 → 404" "404" "$R"

# ------------------------------------------------------------
# RÉSUMÉ
# ------------------------------------------------------------
echo ""
echo "============================================================"
echo -e " ${GREEN}PASS: $PASS${RESET}  |  ${RED}FAIL: $FAIL${RESET}"
echo "============================================================"
echo ""