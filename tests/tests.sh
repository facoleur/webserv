#!/usr/bin/env bash

# I want to be able to:
#	REQUESTS
# 	- test various methods => done
# 	- add whitespace before
# 	- add whitespace after
#	- try various paths => done
#	- add various query-strings => done
#
#	RESPONSES
#	- get the response of webserv
#	- get the response of nginx
#	- evaluate them against each other
#
#	PRINT
#	- print which subset of requests are being tested: valid, invalid, all
#	- print output in various colors
#	- print command that failed

: '
WEBSERV TESTER
Sends requests to webserv and compares the response with NGINX

Usage:
- Run the test: `bash tests.sh`
- Toggle values to obtain the desired tests (valid requests, invalid, all)

Notes:
- Make sure that your NGINX config listens on a different port than your web server
(nginx -t to get the path to your config) 
'

## Starting our webserv and NGINX
# ../webserv
# nginx -s reload

## PORTS
WEBSERV_PORT=8080
NGINX_PORT=8081

## Toggles to obtain different tests
TOGGLE_TESTS=TEST_VALID # Options: TEST_VALID TEST_INVALID TEST_ALL
PRINT_EACH_TEST=TRUE

## Request elements
METHODS_VALID=(GET POST DELETE)
METHODS_INVALID=(UNKNOWN BLABLA /BLABLA)

QUERY_STRINGS_VALID=('' 'foo=bar')
QUERY_STRINGS_VALID=( "${QUERY_STRINGS_VALID[@]/#/?}")
QUERY_STRINGS_INVALID=( "" ) # ????

PATHS_VALID=('' index index.html index.htm)
PATHS_VALID=( "${PATHS_VALID[@]/#//}")
PATHS_INVALID=(' ' / /blabla /blabla.html /blabla.htm)

PROTOCOLS_VALID=(HTTP/1.0 HTTP/1.1)
PROTOCOLS_INVALID=('' ' ' HTTP/0.9 HTTP/2 HTTP)

ENDING_VALID=( $'\r\n\r\n' )
ENDING_INVALID=( $'\r\n' $'\r\n\r\n ' $' \r\n\r\n' $'\r' $'\n' $'' )

ENDING_LABELS_VALID=( '\r\n\r\n' )
ENDING_LABELS_INVALID=( '\r\n' '\r\n\r\n' '\r\n\r\n' '\r' '\n' )

## Selection of request elements
if [[ $TOGGLE_TESTS == "TEST_VALID" ]]; then
	METHODS=("${METHODS_VALID[@]}")
	PATHS=("${PATHS_VALID[@]}")
	QUERY_STRINGS=("${QUERY_STRINGS_VALID[@]}")
	PROTOCOLS=("${PROTOCOLS_VALID[@]}")
	ENDING=("${ENDING_VALID[@]}")
	ENDING_LABELS=("${ENDING_LABELS_VALID[@]}")
	# printf 'Testing valid requests (methods, paths, query strings, protocol versions and ending (CRLF))\n'
elif [[ $TOGGLE_TESTS == "TEST_INVALID" ]]; then
	METHODS=("${METHODS_INVALID[@]}")
	PATHS=("${PATHS_INVALID[@]}")
	QUERY_STRINGS=("${QUERY_STRINGS_INVALID[@]}")
	PROTOCOLS=("${PROTOCOLS_INVALID[@]}")
	ENDING=("${ENDING_INVALID[@]}")
	ENDING_LABELS=("${ENDING_LABELS_INVALID[@]}")
	# printf 'Testing invalid requests (methods, paths, query strings, protocol versions and ending (CRLF))\n'
else
	METHODS=( "${METHODS_VALID[@]}" "${METHODS_INVALID[@]}" )
	PATHS=( "${PATHS_VALID[@]}" "${PATHS_INVALID[@]}" )
	QUERY_STRINGS=( "${QUERY_STRINGS_VALID[@]}" "${QUERY_STRINGS_INVALID[@]}" )
	PROTOCOLS=( "${PROTOCOLS_VALID[@]}" "${PROTOCOLS_INVALID[@]}" )
	ENDING=( "${ENDING_VALID[@]}" "${ENDING_INVALID[@]}" )
	ENDING_LABELS=( "${ENDING_LABELS_VALID[@]}" "${ENDING_LABELS_INVALID[@]}" )
	# printf 'Testing valid and invalid requests (methods, paths, query strings, protocol versions and ending (CRLF))\n'
fi
# printf '************************************************************************************************\n'

## Test a simple GET request with valid ending
# printf 'Test: {%s%s}\n' "${METHODS[0]} ${PATHS[2]} ${PROTOCOLS[0]}" "${ENDING_VALID[0]}"
# printf "%s%s" "${METHODS[0]} ${PATHS[2]} ${PROTOCOLS[0]}" "${ENDING_VALID[0]}" | nc localhost ${WEBSERV_PORT}

## Test a simple GET request with invalid ending
# printf 'Test: %s%s\n' "${METHODS[0]} ${PATHS[2]} ${PROTOCOLS[0]}" "${ENDING_INVALID[0]}"
# printf 'Test: %s%s' "${METHODS[0]} ${PATHS[2]} ${PROTOCOLS[0]}" "${ENDING_INVALID[0]}" | nc localhost ${WEBSERV_PORT}


# Test all possible requests (selected from VALID, INVALID, ALL)
for method in "${METHODS[@]}"; do
	for paths in "${PATHS[@]}"; do
		for protocol in "${PROTOCOLS[@]}"; do
			for idx in "${!ENDING[@]}"; do
				ending=${ENDING[$idx]}
				label=${ENDING_LABELS[$idx]}
				if [[ $PRINT_EACH_TEST == TRUE ]]; then
					printf '*******************\nTest: {%s%s}:\n*******************\n' "$method $paths $protocol" "$label"
				fi
				printf '%s%s' "$method $paths $protocol" "$ending" | nc localhost ${WEBSERV_PORT} # -w 1 
			done
		done
	done
done
