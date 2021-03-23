from typing import List
from server.sockets.web_socket_server import WebSocketServer
from server.tuples import Point


class Logger:
    def __init__(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    def log_server_data(self, data: str):
        print(data)
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})

    def log_drone_data(self, drone_id: str, data: str):
        print(f'{drone_id}: {data}')
        self._web_socket_server.send_message('log', {'name': drone_id, 'message': data})

    def log_map_data(self, drone_id: str, data: List[Point]):
        if len(data) == 0:
            return
        message = f'Points detected by drone {drone_id}: {data}'
        print(message)
        self._web_socket_server.send_message('log', {'name': 'Map', 'message': message})
