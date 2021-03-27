import asyncio
import json
import socket
from typing import Any, Callable, Dict, List, Optional
from server.logger import Logger

EVENT_DENYLIST = {'disconnect'}


class UnixSocketError(Exception):
    pass


class UnixSocketClient:
    def __init__(self, logger: Logger):
        self._logger = logger
        self._callbacks: Dict[str, List[Callable]] = {}
        self._message_queue: asyncio.Queue
        self._create_socket()

    async def serve(self):
        # Initialize message queue here since it must be created in the same event loop as asyncio.run()
        self._message_queue = asyncio.Queue()

        BASE_CONNECTION_TIMEOUT_S = 2
        MAX_CONNECTION_TIMEOUT_S = 8

        timeout_s = BASE_CONNECTION_TIMEOUT_S
        try:
            # Main loop
            while True:
                # Connection attempt loop
                while True:
                    try:
                        await asyncio.get_event_loop().sock_connect(self._socket, '/tmp/hivexplore/socket.sock')
                        self._logger.log_server_data('Connected to ARGoS')
                        timeout_s = BASE_CONNECTION_TIMEOUT_S
                        break
                    except (FileNotFoundError, ConnectionRefusedError, socket.timeout) as exc:
                        self._logger.log_server_data(f'UnixSocketClient connection error: {exc}')
                        self._logger.log_server_data(f'Connection to ARGoS failed, retrying after {timeout_s} seconds')
                        await asyncio.sleep(timeout_s)
                        timeout_s = min(timeout_s * 2, MAX_CONNECTION_TIMEOUT_S)

                # Start send and receive handler tasks
                tasks = [asyncio.create_task(task) for task in [self._receive_handler(), self._send_handler()]]

                try:
                    await asyncio.gather(*tasks)
                except (UnixSocketError, ConnectionResetError) as exc:
                    self._logger.log_server_data(f'UnixSocketClient communication error: {exc}')

                    for callback in self._callbacks.get('disconnect', []):
                        callback()

                    for task in tasks:
                        task.cancel()
                    self._socket.close()
                    self._create_socket()
        finally:
            self._socket.close()

    def bind(self, log_name: str, callback: Callable[[Optional[str], Any], None]):
        self._callbacks.setdefault(log_name, []).append(callback)

    def send(self, param_name: str, drone_id: str, value: Any):
        self._message_queue.put_nowait({
            'paramName': param_name,
            'droneId': drone_id,
            'value': value,
        })

    def _create_socket(self):
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        self._socket.setblocking(False)

    async def _receive_handler(self):
        while True:
            message_bytes = await asyncio.get_event_loop().sock_recv(self._socket, 4096)

            if len(message_bytes) == 0:
                raise UnixSocketError('Socket connection broken in receive handler')

            try:
                message = json.loads(message_bytes.decode('utf-8'))

                if message['logName'] in EVENT_DENYLIST:
                    self._logger.log_server_data(f'UnixSocketClient error: Invalid event received: {message["logName"]}')
                    continue

                try:
                    callbacks = self._callbacks[message['logName']]
                except KeyError:
                    self._logger.log_server_data(f'UnixSocketClient warning: No callbacks bound for log name: {message["logName"]}')
                    continue

                for callback in callbacks:
                    callback(message['droneId'], message['variables'])
            except (json.JSONDecodeError, KeyError) as exc:
                self._logger.log_server_data(f'UnixSocketClient error: Invalid message received: {exc}')

    async def _send_handler(self):
        while True:
            message = await self._message_queue.get()
            message_str = json.dumps(message)
            sent = await asyncio.get_event_loop().sock_sendall(self._socket, message_str.encode('utf-8'))

            if sent == 0:
                raise UnixSocketError('Socket connection broken in send handler')
