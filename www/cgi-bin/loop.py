#!/usr/bin/env python3
import sys
import time


def main():
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    sys.stdout.write("This CGI script will never finish.\n")
    sys.stdout.flush()
    while True:
        time.sleep(1)


if __name__ == "__main__":
    main()
