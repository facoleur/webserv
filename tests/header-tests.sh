#!/usr/bin/env bash

: '
HEADERS PARSING TESTER
'

## Toggles to obtain different tests
TOGGLE_TESTS=TEST_ALL # Options: TEST_VALID TEST_INVALID TEST_ALL
PRINT_EACH_TEST=TRUE

## Headers elements
HEADERS_NAME_VALID=(Host Content-length Content-type Transfer-encoding foo)
HEADERS_NAME_INVALID=('foo :' ' foo:' 'foo')

HEADERS_FIELD_VALID=('localhost' '      localhost' 'local host' 'localhost      ' 'chunked' 'CHUNKED')
HEADERS_FIELD_INVALID=(':localhost' 'localhost:' '::localhost')

ENDING=( $'\r\n\r\n' )
SEPARATOR=( ':' )

## Selection of request elements
if [[ $TOGGLE_TESTS == "TEST_VALID" ]]; then
	HEADERS_NAMES=("${HEADERS_NAME_VALID[@]}")
	HEADERS_FIELDS=("${HEADERS_FIELD_VALID[@]}")
elif [[ $TOGGLE_TESTS == "TEST_INVALID" ]]; then
	HEADERS_NAMES=("${HEADERS_NAME_INVALID[@]}")
	HEADERS_FIELDS=("${HEADERS_FIELD_INVALID[@]}")
else
	HEADERS_NAMES=( "${HEADERS_NAME_VALID[@]}" "${HEADERS_NAME_INVALID[@]}" )
	HEADERS_FIELDS=( "${HEADERS_FIELD_VALID[@]}" "${HEADERS_FIELD_INVALID[@]}" )
fi

## Test a simple valid request with one header - Host: example.com
# printf 'GET / HTTP/1.1\r\nHost: example.com\r\nFoo:  bar \r\n\r\n' \  | ./header_tester

# Test all possible requests (selected from VALID, INVALID, ALL)
for header_name in "${HEADERS_NAMES[@]}"; do
	for header_field in "${HEADERS_FIELDS[@]}"; do
		if [[ $PRINT_EACH_TEST == TRUE ]]; then
			printf '*****\nTest: {%s%s%s%s}:\n' "$header_name" "$SEPARATOR" "$header_field" "$ENDING"
		fi
			printf '%s%s%s%s' "$header_name" "$SEPARATOR" "$header_field" "$ENDING" | ./header_tester
	done
done
