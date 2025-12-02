#!/bin/sh

WEBSERV="http://127.0.0.1:9080"
NGINX="http://127.0.0.1:8080"

LOGFILE="diff_results.log"
: >"$LOGFILE"

GREEN="\033[0;32m"
RED="\033[0;31m"
RESET="\033[0m"

# ============ Helpers ============

request() {
    method="$1"
    url="$2"
    data="$3"

    TMP_BODY=$(mktemp)

    case "$method" in
    GET)
        STATUS=$(curl -s -o "$TMP_BODY" -w "%{http_code}" "$url")
        ;;
    POST)
        STATUS=$(curl -s -X POST $data -o "$TMP_BODY" -w "%{http_code}" "$url")
        ;;
    DELETE)
        STATUS=$(curl -s -X DELETE -o "$TMP_BODY" -w "%{http_code}" "$url")
        ;;
    esac

    BODY_LEN=$(wc -c <"$TMP_BODY")

    # Use safe delimiter:
    echo "${STATUS}|${BODY_LEN}|${TMP_BODY}"
}

compare() {
    method="$1"
    url="$2"
    ws_expect="$3"
    ngx_expect="$4"
    compare_body="$5"

    WEBSERV_OUT=$(request "$method" "$WEBSERV$url" "")
    NGINX_OUT=$(request "$method" "$NGINX$url" "")

    # SAFE PARSING
    IFS="|" read WS_STATUS WS_LEN WS_BODY <<EOF
$WEBSERV_OUT
EOF

    IFS="|" read NX_STATUS NX_LEN NX_BODY <<EOF
$NGINX_OUT
EOF

    # Check status per-server
    status_ok=true
    if [ "$WS_STATUS" != "$ws_expect" ] || [ "$NX_STATUS" != "$ngx_expect" ]; then
        status_ok=false
    fi

    # Compare body-length or full body
    body_ok=true

    if [ "$compare_body" = "1" ]; then
        # Compare full bodies
        if ! diff -u "$WS_BODY" "$NX_BODY" >/dev/null; then
            body_ok=false
        fi
    else
        # Compare only lengths
        if [ "$WS_LEN" != "$NX_LEN" ]; then
            body_ok=false
        fi
    fi

    # log output
    {
        echo "==== $method $url ===="
        echo "Webserv: $WS_STATUS (expected $ws_expect)"
        echo "Nginx:   $NX_STATUS (expected $ngx_expect)"
        echo
        if [ "$compare_body" = "1" ]; then
            echo "Body diff:"
            diff -u "$WS_BODY" "$NX_BODY" || true
            echo
        fi
    } >>"$LOGFILE"

    rm -f "$WS_BODY" "$NX_BODY"

    # === Colored output ===
    printf "%-6s %-30s" "$method" "$url"

    if $status_ok; then
        printf "${GREEN}OK${RESET}"
    else
        printf "${RED}KO${RESET}"
    fi

    printf "     "

    if $body_ok; then
        printf "${GREEN}OK${RESET}"
    else
        printf "${RED}KO${RESET}"
    fi

    printf "\n"
}

### --- Static File Tests ---
test_static() {
    compare GET /index.html 200 200 1
    compare GET /does_not_exist 404 404 0
    compare GET / 200 200 1
}

### --- Directory listing / no index ---
test_directory() {
    compare GET /autoindex 301 301 0
    compare GET /noautoindex/ 403 403 0
}


### --- Upload (POST) ---
test_upload() {
    compare POST /upload "204" "204" 0
    compare POST /upload "204" "204" 0
    compare POST /upload "204" "204" 0
}

### --- DELETE (DAV-like) ---
test_delete_dav() {

    mkdir -p "delete_ws"
    mkdir -p "delete_nx"

    echo "a" >"delete/a"
    echo "a" >"delete/a"

    compare DELETE /delete/a 204 404 0
    compare DELETE /delete/a 404 404 0
    compare DELETE /delete/nope 404 404 0
}

### --- CGI Tests ---
test_cgi() {
    compare GET "/cgi/noexist.py" 404 404 0
    compare GET "/cgi/error.py" 500 403 1
    # compare GET "/cgi/script.py?x=1&y=2" 200 200 1
    # compare POST /cgi/echo.py 200 200 1
    # compare POST /cgi/echo.py 200 200 1
}

### --- Error Pages ---
test_errors() {
    compare POST /index.html 405 405 0
}

# ============ Execute ============

mkdir -p delete
echo "a" >delete/a_ws
echo "a" >delete/a_nx

test_static
test_directory
test_upload
test_delete_dav
test_cgi
test_errors

echo "Done. See $LOGFILE"
