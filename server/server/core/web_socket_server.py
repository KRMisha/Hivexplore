import asyncio
from datetime import datetime
import json
from typing import Any, Callable, Dict, List
import websockets

IP_ADDRESS = 'localhost'
PORT = 5678


class WebSocketServer:
    def __init__(self):
        self._callbacks: Dict[str, List[Callable]] = {}
        self._message_queue = None

    async def serve(self):
        # Initialize message queue here since it must be created in the same event loop as asyncio.run()
        self._message_queue = asyncio.Queue()

        server = await websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        print('WebSocketServer started')
        await server.wait_closed()

    def send(self, event: str, drone_id: str, data: Any):
        self._message_queue.put_nowait({'event': event, 'droneId': drone_id, 'data': data, 'timestamp': datetime.now().isoformat()})

    def bind(self, event: str, callback: Callable[[Any], None]):
        self._callbacks.setdefault(event, []).append(callback)

    async def _socket_handler(self, websocket, path):
        print('New client connected:', websocket.origin)
        receive_task = asyncio.create_task(self._receive_handler(websocket, path))
        send_task = asyncio.create_task(self._send_handler(websocket, path))

        _done, pending = await asyncio.wait([receive_task, send_task], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()

        print('Client disconnected:', websocket.origin)

    async def _receive_handler(self, websocket, _path):
        async for message_str in websocket:
            try:
                message = json.loads(message_str)
                for callback in self._callbacks.get(message['event'], []):
                    callback(message['data'])
            except (json.JSONDecodeError, KeyError) as exc:
                print('WebSocketServer error: Invalid message received:', exc)

    async def _send_handler(self, websocket, _path):
        while True:
            message = await self._message_queue.get()
            message_str = json.dumps(message)
            await websocket.send(message_str)
