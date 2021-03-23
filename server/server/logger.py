from server.sockets.web_socket_server import WebSocketServer


class Logger:
    def __init__(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server

    def log_server_data(self, data: str):
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})

    def log_drone_data(self, drone_id: str, data: str):
        self._web_socket_server.send_message('log', {'name': drone_id, 'message': data})

    def log_map_data(self, data: str):
        self._web_socket_server.send_message('log', {'name': 'Server', 'message': data})
