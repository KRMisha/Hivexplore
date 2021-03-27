from __future__ import annotations

import logging
from typing import List, TYPE_CHECKING
from server.tuples import Point
if TYPE_CHECKING:
    from server.sockets.web_socket_server import WebSocketServer


class Logger:
    def __init__(self):
        self._web_socket_server: WebSocketServer

    def set_web_socket_server(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    @staticmethod
    def log_server_local_data(data: str):
        logging.info(data)

    def log_server_data(self, data: str):
        logging.info(data)
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})

    def log_drone_data(self, drone_id: str, data: str):
        logging.info(f'{drone_id}: {data}')
        self._web_socket_server.send_message('log', {'name': drone_id, 'message': data})

    def log_map_data(self, drone_id: str, data: List[Point]):
        if len(data) == 0:
            return
        message = f'Points detected by drone {drone_id}: {data}'
        logging.info(message)
        self._web_socket_server.send_message('log', {'name': 'Map', 'message': message})
