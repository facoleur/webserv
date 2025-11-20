#!/usr/bin/env python3
import os
import sys

print("Status: 200 OK")
print("Content-Type: text/plain\r")
print("\r")
print("Hello from CGI!\n")
print("Method: {}".format(os.environ.get("REQUEST_METHOD", "")))
print("Query: {}".format(os.environ.get("QUERY_STRING", "")))
body = sys.stdin.read()
if body:
    print("Body: {}".format(body))
