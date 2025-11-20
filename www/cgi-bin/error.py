#!/usr/bin/env python3
import sys


def main():
    sys.stdout.write("Status: 500 Internal Server Error\r\n")
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    sys.stdout.write("This CGI script failed intentionally.\n")
    sys.stderr.write("error.py exiting with code 1\n")
    sys.stderr.flush()
    sys.exit(1)


if __name__ == "__main__":
    main()
