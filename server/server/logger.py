from __future__ import annotations

from datetime import datetime
import logging
from logging import config
from typing import List, TYPE_CHECKING
import yaml
from server.tuples import Point
if TYPE_CHECKING:
    from server.sockets.web_socket_server import WebSocketServer

log_filename = f'logs/hivexplore_logs_{datetime.now().isoformat().replace(":", "")}.log'


class DebugInfoFilter(logging.Filter):
    @staticmethod
    def filter(record):
        return record.levelno in (logging.DEBUG, logging.INFO)


class Logger:
    def __init__(self):
        self._web_socket_server: WebSocketServer
        with open('server/logging_config.yml', 'r') as file:
            config_dict = yaml.load(file, Loader=yaml.FullLoader)
            config.dictConfig(config_dict)
        self._logger = logging.getLogger('hivexplore')

    def set_web_socket_server(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    def log_server_local_data(self, data: str):
        self._logger.info(data)

    def log_server_data(self, data: str):
        self._logger.info(data)
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})

    def log_drone_data(self, drone_id: str, data: str):
        self._logger.info(drone_id + ': ' + data)
        self._web_socket_server.send_message('log', {'name': drone_id, 'message': data})

    def log_map_data(self, drone_id: str, data: List[Point]):
        if len(data) == 0:
            return
        message = f'Points detected by drone {drone_id}: {data}'
        self._logger.info(message)
        self._web_socket_server.send_message('log', {'name': 'Map', 'message': message})
