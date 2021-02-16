import asyncio
import websockets

IP_ADDRESS = 'localhost'
PORT = 5678


# TODO: Complete
class SocketServer:
    def __init__(self):
        self._callbacks = [] # TODO: Make this a dict to sort callbacks by message type (dict of lists)

    def serve(self):
        server_instance = websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        asyncio.get_event_loop().run_until_complete(server_instance)
        asyncio.get_event_loop().run_forever()

    # TODO: Add message type parameter
    def add_callback(self, callback):
        self._callbacks.append(callback)

    def send(self, message):
        pass

    async def _socket_handler(self, websocket, path):
        pass
