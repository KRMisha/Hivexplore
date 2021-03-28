from __future__ import annotations

from datetime import datetime
import logging
from logging import config
from pathlib import Path
from typing import List, TYPE_CHECKING
import yaml
from server.tuples import Point
if TYPE_CHECKING:
    from server.sockets.web_socket_server import WebSocketServer

timestamp = datetime.now().isoformat().replace(':', '')
log_filename = Path('logs') / Path(f'hivexplore_logs_{timestamp}.log')
logger = logging.getLogger('hivexplore')


def setup_logger():
    with open(Path('server') / Path('logging_config.yml'), 'r') as stream:
        my_config = yaml.load(stream, Loader=yaml.FullLoader)
        config.dictConfig(my_config)


class Logger:
    def __init__(self):
        self._web_socket_server: WebSocketServer

    def set_web_socket_server(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    @staticmethod
    def log_server_local_data(data: str):
        logger.info(data)

    def log_server_data(self, data: str):
        logger.info(data)
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})

    def log_drone_data(self, drone_id: str, data: str):
        logger.info(drone_id + ': ' + data)
        self._web_socket_server.send_message('log', {'name': drone_id, 'message': data})

    def log_map_data(self, drone_id: str, data: List[Point]):
        if len(data) == 0:
            return
        message = f'Points detected by drone {drone_id}: {data}'
        logger.info(message)
        self._web_socket_server.send_message('log', {'name': 'Map', 'message': message})
