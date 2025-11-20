#!/usr/bin/env python3
import sys


def main():
    sys.stdout.write("Malformed header without colon\r\n")
    sys.stdout.write("Still missing separator\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write("This body intentionally ships without valid headers.\n")


if __name__ == "__main__":
    main()
