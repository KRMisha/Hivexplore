import asyncio
import websockets

IP_ADDRESS = 'localhost'
PORT = 5678


# TODO: Complete
class SocketServer:
    def __init__(self):
        self._callbacks = [] # TODO: Make this a dict to sort callbacks by message type (dict of lists)
        self._message_queue = asyncio.Queue()

    def serve(self):
        server_instance = websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        asyncio.get_event_loop().run_until_complete(server_instance)
        asyncio.get_event_loop().run_forever()

    def send(self, message):
        self._message_queue.put(message)

    # TODO: Add message type parameter
    def bind(self, callback):
        self._callbacks.append(callback)

    async def _socket_handler(self, websocket, path):
        receive_task = asyncio.ensure_future(self._receive_handler(websocket, path))
        send_task = asyncio.ensure_future(self._send_handler(websocket, path))

        done, pending = await asyncio.wait([receive_task, send_task], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()

    async def _receive_handler(self, websocket, path):
        async for message in websocket:
            for callback in self._callbacks:
                callback(message)

    async def _send_handler(self, websocket, path):
        while True:
            message = await self._message_queue.get()
            await websocket.send(message)
