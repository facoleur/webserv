#!/bin/sh

SERVER_A="http://127.0.0.1:8080"
SERVER_B="http://127.0.0.1:8081"

LOGFILE="diff_results.log"
: > "$LOGFILE"

URLS="/index.html /dir /www"
METHODS="GET POST DELETE"


run_test() {
    method="$1"
    url="$2"

    A_BODY=$(mktemp)
    B_BODY=$(mktemp)
    A_STATUS_FILE=$(mktemp)
    B_STATUS_FILE=$(mktemp)
    A_LEN_FILE=$(mktemp)
    B_LEN_FILE=$(mktemp)

    case "$method" in
        GET)
            A_STATUS=$(curl -s -o "$A_BODY" -w "%{http_code}" "$SERVER_A$url")
            B_STATUS=$(curl -s -o "$B_BODY" -w "%{http_code}" "$SERVER_B$url")
            ;;
        POST)
            A_STATUS=$(curl -s -X POST -d '' -o "$A_BODY" -w "%{http_code}" "$SERVER_A$url")
            B_STATUS=$(curl -s -X POST -d '' -o "$B_BODY" -w "%{http_code}" "$SERVER_B$url")
            ;;
        DELETE)
            A_STATUS=$(curl -s -X DELETE -o "$A_BODY" -w "%{http_code}" "$SERVER_A$url")
            B_STATUS=$(curl -s -X DELETE -o "$B_BODY" "$SERVER_B$url")
            ;;
    esac

    A_LEN=$(wc -c < "$A_BODY")
    B_LEN=$(wc -c < "$B_BODY")

    printf "%s %s: " "$method" "$url"

    ok=true

    # write status and lengths to temp files
    printf "%s" "$A_STATUS" > "$A_STATUS_FILE"
    printf "%s" "$B_STATUS" > "$B_STATUS_FILE"
    printf "%s" "$A_LEN" > "$A_LEN_FILE"
    printf "%s" "$B_LEN" > "$B_LEN_FILE"

    {
        echo "==== $method $url status ===="
        diff -u "$A_STATUS_FILE" "$B_STATUS_FILE" || ok=false

        echo "==== $method $url length ===="
        diff -u "$A_LEN_FILE" "$B_LEN_FILE" || ok=false

        echo "==== $method $url body ===="
        diff -u "$A_BODY" "$B_BODY" || ok=false

        echo
    } >> "$LOGFILE"

    rm "$A_BODY" "$B_BODY" "$A_STATUS_FILE" "$B_STATUS_FILE" "$A_LEN_FILE" "$B_LEN_FILE"

    if $ok; then
        echo "OK"
    else
        echo "KO"
    fi
}

for url in $URLS; do
    for method in $METHODS; do
        run_test "$method" "$url"
    done
done

echo "Diffs saved in $LOGFILE"
