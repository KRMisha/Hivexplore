import asyncio
from datetime import datetime
import json
from typing import Any, Callable, Dict, List
import websockets

IP_ADDRESS = 'localhost'
PORT = 5678


class SocketServer:
    def __init__(self):
        self._callbacks: Dict[str, List[Callable]] = {}
        self._message_queue = asyncio.Queue()

    def serve(self):
        server_instance = websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        asyncio.get_event_loop().run_until_complete(server_instance)
        asyncio.get_event_loop().run_forever()

    def send(self, event: str, data: Any):
        self._message_queue.put_nowait({'event': event, 'data': data, 'timestamp': datetime.now().isoformat()})

    def bind(self, event: str, callback: Callable[[Any], None]):
        self._callbacks.setdefault(event, []).append(callback)

    async def _socket_handler(self, websocket, path):
        receive_task = asyncio.ensure_future(self._receive_handler(websocket, path))
        send_task = asyncio.ensure_future(self._send_handler(websocket, path))

        _done, pending = await asyncio.wait([receive_task, send_task], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()

    async def _receive_handler(self, websocket, _path):
        async for message_str in websocket:
            message = json.loads(message_str)
            for callback in self._callbacks.get(message['event'], []):
                callback(message['data'])

    async def _send_handler(self, websocket, _path):
        while True:
            message = await self._message_queue.get()
            message_str = json.dumps(message)
            await websocket.send(message_str)
