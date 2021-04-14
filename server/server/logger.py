from __future__ import annotations

from datetime import datetime
import logging
import logging.config
from pathlib import Path
from typing import List, TYPE_CHECKING
import yaml
from server.communication.web_socket_event import WebSocketEvent
from server.tuples import Point
if TYPE_CHECKING:
    from server.communication.web_socket_server import WebSocketServer

LOGS_DIRECTORY = Path('logs')
log_filename = '' # pylint: disable=invalid-name


class DebugInfoFilter(logging.Filter):
    @staticmethod
    def filter(record):
        return record.levelno in (logging.DEBUG, logging.INFO)


class Logger:
    def __init__(self):
        self._web_socket_server: WebSocketServer
        LOGS_DIRECTORY.mkdir(exist_ok=True)
        self.setup_logging() # Setup logging for the first mission

    def setup_logging(self):
        global log_filename # pylint: disable=global-statement, invalid-name
        log_filename = LOGS_DIRECTORY / f'hivexplore_{datetime.now().isoformat().replace(":", "")}.log'
        with open('server/logging_config.yml', 'r') as file:
            config = yaml.load(file, Loader=yaml.FullLoader)
            logging.config.dictConfig(config)
        self._logger = logging.getLogger('hivexplore')

    def set_web_socket_server(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    def log_server_local_data(self, level: int, data: str):
        self._logger.log(level, data)

    def log_server_data(self, level: int, data: str):
        self._logger.log(level, data)
        self._web_socket_server.send_message(WebSocketEvent.LOG, {'group': 'Server', 'line': data})

    def log_drone_data(self, level: int, drone_id: str, data: str):
        self._logger.log(level, f'{drone_id}: {data}') # pylint: disable=logging-fstring-interpolation
        self._web_socket_server.send_message(WebSocketEvent.LOG, {'group': drone_id, 'line': data})

    def log_map_data(self, level: int, drone_id: str, data: List[Point]):
        if len(data) == 0:
            return
        message = f'Points detected by drone {drone_id}: {data}'
        self._logger.log(level, message)
        self._web_socket_server.send_message(WebSocketEvent.LOG, {'group': 'Map', 'line': message})
