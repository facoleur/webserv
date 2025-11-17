#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER_BIN="${ROOT_DIR}/webserv"
CONFIG_FILE="${ROOT_DIR}/config/cgi_test.conf"
PORT=8085
LOG_DIR="${ROOT_DIR}/tests/logs"
mkdir -p "${LOG_DIR}"
SERVER_LOG="${LOG_DIR}/cgi_server.log"

SERVER_PID=""

cleanup() {
    if [[ -n "${SERVER_PID}" ]]; then
        if kill -0 "${SERVER_PID}" >/dev/null 2>&1; then
            kill "${SERVER_PID}" >/dev/null 2>&1 || true
        fi
        wait "${SERVER_PID}" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

start_server() {
    "${SERVER_BIN}" "${CONFIG_FILE}" >"${SERVER_LOG}" 2>&1 &
    SERVER_PID=$!
    sleep 0.5
}

assert_contains() {
    local needle="$1"
    local file="$2"
    if ! grep -q "${needle}" "${file}"; then
        echo "    ✗ did not find '${needle}' in ${file}"
        return 1
    fi
    return 0
}

run_request() {
    local name="$1"
    local method="$2"
    local path="$3"
    local data="${4:-}"
    local expected_status="$5"
    local expect_body_fragment="$6"

    local headers
    local body
    headers="$(mktemp)"
    body="$(mktemp)"

    local curl_args=(--max-time 5 -sS -D "${headers}" -o "${body}" -X "${method}")
    if [[ -n "${data}" ]]; then
        curl_args+=(--data "${data}")
    fi
    curl_args+=("http://127.0.0.1:${PORT}${path}")

    if ! curl "${curl_args[@]}" >/dev/null; then
        echo "[${name}] ✗ curl failed"
        rm -f "${headers}" "${body}"
        return 1
    fi

    local status
    status="$(head -n1 "${headers}" | awk '{print $2}')"
    if [[ "${status}" != "${expected_status}" ]]; then
        echo "[${name}] ✗ Expected status ${expected_status}, got ${status}"
        echo "Response headers:"
        cat "${headers}"
        rm -f "${headers}" "${body}"
        return 1
    fi

    if ! assert_contains "${expect_body_fragment}" "${body}"; then
        echo "[${name}] ✗ Expected response body to contain '${expect_body_fragment}'"
        cat "${body}"
        rm -f "${headers}" "${body}"
        return 1
    fi

    echo "[${name}] ✓ status ${status}"

    rm -f "${headers}" "${body}"
    return 0
}

run_malformed_header_check() {
    local headers
    headers="$(mktemp)"
    local body
    body="$(mktemp)"
    if ! curl -sS -D "${headers}" -o "${body}" "http://127.0.0.1:${PORT}/cgi-bin/malformed.py" >/dev/null; then
        echo "[malformed] ✗ curl failed"
        rm -f "${headers}" "${body}"
        return 1
    fi
    if ! grep -i "Content-Type: text/html" "${headers}" >/dev/null; then
        echo "[malformed] ✗ Expected fallback Content-Type header"
        cat "${headers}"
        rm -f "${headers}" "${body}"
        return 1
    fi
    echo "[malformed] ✓ fallback Content-Type applied"
    rm -f "${headers}" "${body}"
}

run_slow_script_probe() {
    local start
    start=$(date +%s)
    if curl --max-time 2 -sS "http://127.0.0.1:${PORT}/cgi-bin/slow.py" >/dev/null; then
        echo "[slow] ✓ script completed within 2s"
    else
        local exit_code=$?
        local duration=$(( $(date +%s) - start ))
        echo "[slow] ⚠ curl exited with ${exit_code} after ${duration}s (indicates lack of CGI timeout handling)"
    fi
}

main() {
    start_server

    local overall_status=0

    run_request "cgiget" "GET" "/cgi-bin/echo.py?foo=bar" "" "200" "Query=foo=bar" || overall_status=1
    run_request "cgipost" "POST" "/cgi-bin/echo.py" "name=Codex" "200" "Body=name=Codex" || overall_status=1
    run_request "cgierror" "GET" "/cgi-bin/error.py" "" "502" "CGI script" || overall_status=1
    run_malformed_header_check || overall_status=1
    run_slow_script_probe || true

    exit "${overall_status}"
}

main "$@"
