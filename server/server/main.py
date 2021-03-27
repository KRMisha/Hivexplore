from datetime import datetime
import logging
# TODO: Remove me, but bug from https://stackoverflow.com/questions/64951836/python-logging-attributeerror-module-logging-has-no-attribute-handlers
from logging.handlers import RotatingFileHandler
from logging import StreamHandler
from pathlib import Path
import sys
from server.server import Server


def main():
    try:
        timestamp = datetime.now().isoformat().replace(':', '')
        file_handler = RotatingFileHandler(filename=Path('logs') / Path(f'hivexplore_logs_{timestamp}.log'), maxBytes=512*1024, backupCount=10)
        stream_handler = StreamHandler()
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s [%(levelname)s]\t%(message)s',
            handlers=[
                file_handler,
                stream_handler,
            ]
        )

        is_argos_simulation = 'argos' in sys.argv[1:]
        enable_debug_driver = 'debug' in sys.argv[1:]
        server = Server(is_argos_simulation, enable_debug_driver)
        server.start()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
