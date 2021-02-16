#!/usr/bin/env python3

import sys
from server.core.server import Server


def main():
    enable_debug_driver = 'debug' in sys.argv[1:]
    print(enable_debug_driver)

    server = Server(enable_debug_driver)
    server.start()


if __name__ == '__main__':
    main()
