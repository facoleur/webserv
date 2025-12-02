#!/usr/bin/env python3
import os
import sys


def print_header(name, value):
    sys.stdout.write(f"{name}: {value}\r\n")


method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")
body = sys.stdin.read()

print_header("Status", "200 OK")
print_header("Content-Type", "text/plain; charset=utf-8")
print_header("X-Debug-Method", method)
print_header("X-Debug-Query", query)
sys.stdout.write("\r\n")
print("Hello from CGI echo!")
print(f"Method={method}")
print(f"Query={query}")
if body:
    print(f"Body={body}")
else:
    print("Body=")
