from __future__ import annotations

import asyncio
from datetime import datetime
import json
from typing import Any, Callable, Dict, List, Optional, TYPE_CHECKING, Union
import uuid
import websockets
if TYPE_CHECKING:
    from server.logger import Logger

IP_ADDRESS = ''
PORT = 5678
EVENT_DENYLIST = {'connect'}


class WebSocketServer:
    def __init__(self, logger: Logger):
        self._logger = logger
        self._callbacks: Dict[str, List[Callable]] = {}
        self._message_queues: Dict[str, asyncio.Queue] = {}
        self._loop: asyncio.AbstractEventLoop

    async def serve(self):
        self._loop = asyncio.get_running_loop()
        server = await websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        self._logger.log_server_local_data('WebSocketServer started')
        await server.wait_closed()

    def bind(self, event: str, callback: Union[Callable[[Any], None], Callable[[str, Any], None]]):
        self._callbacks.setdefault(event, []).append(callback)

    def send_message(self, event: str, data: Any):
        # None represents an event not related to a specific drone
        self._send(event, None, data)

    def send_drone_message(self, event: str, drone_id: str, data: Any):
        self._send(event, drone_id, data)

    def send_message_to_client(self, client_id, event: str, data: Any):
        self._send_to_client(client_id, event, None, data)

    def send_drone_message_to_client(self, client_id, event: str, drone_id: str, data: Any):
        self._send_to_client(client_id, event, drone_id, data)

    def _send(self, event: str, drone_id: Optional[str], data: Any):
        for message_queue in self._message_queues.values():
            asyncio.run_coroutine_threadsafe(
                message_queue.put({
                    'event': event,
                    'droneId': drone_id,
                    'data': data,
                    'timestamp': datetime.now().isoformat(),
                }), self._loop)

    def _send_to_client(self, client_id: str, event: str, drone_id: Optional[str], data: Any):
        if client_id not in self._message_queues:
            self._logger.log_server_local_data(f'WebSocketServer error: Unknown client ID: {client_id}')
            return
        asyncio.run_coroutine_threadsafe(
            self._message_queues[client_id].put({
                'event': event,
                'droneId': drone_id,
                'data': data,
                'timestamp': datetime.now().isoformat(),
            }), self._loop)

    async def _socket_handler(self, websocket, path):
        # Generate a unique ID for each client
        client_id = uuid.uuid4().hex

        self._logger.log_server_local_data(f'New client connected: {client_id}')

        self._message_queues[client_id] = asyncio.Queue()

        for callback in self._callbacks.get('connect', []):
            callback(client_id)

        receive_task = asyncio.create_task(self._receive_handler(websocket, path))
        send_task = asyncio.create_task(self._send_handler(websocket, path, self._message_queues[client_id]))

        _done, pending = await asyncio.wait([receive_task, send_task], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()

        self._logger.log_server_local_data(f'Client disconnected: {client_id}')

        del self._message_queues[client_id]

    async def _receive_handler(self, websocket, _path):
        async for message_str in websocket:
            try:
                message = json.loads(message_str)

                if message['event'] in EVENT_DENYLIST:
                    self._logger.log_server_local_data(f'WebSocketServer error: Invalid event received: {message["event"]}')
                    continue

                try:
                    callbacks = self._callbacks[message['event']]
                except KeyError:
                    self._logger.log_server_local_data(f'WebSocketServer warning: No callbacks bound for event: {message["event"]}')
                    continue

                for callback in callbacks:
                    if message['droneId'] is None:
                        callback(message['data'])
                    else:
                        callback(message['droneId'], message['data'])
            except (json.JSONDecodeError, KeyError) as exc:
                self._logger.log_server_local_data(f'WebSocketServer error: Invalid message received: {exc}')

    async def _send_handler(self, websocket, _path, message_queue):
        while True:
            message = await message_queue.get()
            message_str = json.dumps(message)
            await websocket.send(message_str)
