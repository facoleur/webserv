#!/usr/bin/env python3
import sys
import time


def main():
    time.sleep(5)
    sys.stdout.write("Status: 200 OK\r\n")
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    sys.stdout.write("Slow CGI finished after sleeping.\n")


if __name__ == "__main__":
    main()
