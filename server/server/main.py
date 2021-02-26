#!/usr/bin/env python3

import sys
from server.core.server import Server


def main():
    try:
        enable_debug_driver = 'debug' in sys.argv[1:]
        is_argos_simulation = 'argos' in sys.argv[1:]
        server = Server(enable_debug_driver, is_argos_simulation)
        server.start()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
