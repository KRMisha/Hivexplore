import asyncio
import json
import logging
import socket
from typing import Any, Callable, Dict, List, Optional, Union
from server.communication.log_name import LogName
from server.communication.unix_socket_event import UnixSocketEvent
from server.logger.logger import Logger

EVENT_DENYLIST = {UnixSocketEvent.DISCONNECT}


class UnixSocketError(Exception):
    pass


class UnixSocketClient:
    def __init__(self, logger: Logger):
        self._logger = logger
        self._callbacks: Dict[Union[LogName, UnixSocketEvent], List[Callable]] = {}
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
                        self._logger.log_server_data(logging.INFO, 'Connected to ARGoS')
                        timeout_s = BASE_CONNECTION_TIMEOUT_S
                        break
                    except (FileNotFoundError, ConnectionRefusedError, socket.timeout) as exc:
                        self._logger.log_server_data(logging.ERROR, f'UnixSocketClient connection error: {exc}')
                        self._logger.log_server_data(logging.WARN, f'Connection to ARGoS failed, retrying after {timeout_s} seconds')
                        await asyncio.sleep(timeout_s)
                        timeout_s = min(timeout_s * 2, MAX_CONNECTION_TIMEOUT_S)

                # Start send and receive handler tasks
                tasks = [asyncio.create_task(task) for task in [self._receive_handler(), self._send_handler()]]

                try:
                    await asyncio.gather(*tasks)
                except (UnixSocketError, ConnectionResetError) as exc:
                    self._logger.log_server_data(logging.ERROR, f'UnixSocketClient communication error: {exc}')

                    for callback in self._callbacks.get(UnixSocketEvent.DISCONNECT, []):
                        callback()

                    for task in tasks:
                        task.cancel()
                    self._socket.close()
                    self._create_socket()
        finally:
            self._socket.close()

    def bind(self, log_name: Union[LogName, UnixSocketEvent], callback: Callable[[Optional[str], Any], None]):
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

                try:
                    log_name = LogName(message['logName'])
                except ValueError:
                    self._logger.log_server_data(logging.WARN, f'UnixSocketClient warning: Invalid log name received: {message["logName"]}')
                    continue

                if log_name in EVENT_DENYLIST:
                    self._logger.log_server_data(logging.ERROR,
                                                 f'UnixSocketClient error: Forbidden log name received: {message["logName"]}')
                    continue

                try:
                    callbacks = self._callbacks[log_name]
                except KeyError:
                    self._logger.log_server_data(logging.WARN,
                                                 f'UnixSocketClient warning: No callbacks bound for log name: {message["logName"]}')
                    continue

                for callback in callbacks:
                    callback(message['droneId'], message['variables'])

            except (json.JSONDecodeError, KeyError) as exc:
                self._logger.log_server_data(logging.ERROR, f'UnixSocketClient error: Invalid message received: {exc}')

    async def _send_handler(self):
        while True:
            message = await self._message_queue.get()
            try:
                message_str = json.dumps(message)
            except TypeError as exc:
                self._logger.log_server_data(logging.ERROR, f'UnixSocketClient error: Unable to serialize: {exc}')

            sent = await asyncio.get_event_loop().sock_sendall(self._socket, message_str.encode('utf-8'))

            if sent == 0:
                raise UnixSocketError('Socket connection broken in send handler')
