import sys
from server.logger import setup_logger
from server.server import Server


def main():
    try:
        setup_logger()
        is_argos_simulation = 'argos' in sys.argv[1:]
        enable_debug_driver = 'debug' in sys.argv[1:]
        server = Server(is_argos_simulation, enable_debug_driver)
        server.start()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
