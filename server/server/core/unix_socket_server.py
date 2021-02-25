import asyncio
import json
import socket
from typing import Any, Callable, Dict, List
from server import config


class UnixSocketError(Exception):
    pass


class UnixSocketServer:
    def __init__(self):
        self._callbacks: Dict[str, List[Callable]] = {}
        self._message_queue = None
        self._create_socket()

    async def serve(self):
        # Initialize message queue here since it must be created in the same event loop as asyncio.run()
        self._message_queue = asyncio.Queue()

        timeout_s = config.BASE_CONNECTION_TIMEOUT_S
        try:
            # Main loop
            while True:
                # Connection attempt loop
                while True:
                    try:
                        await asyncio.get_event_loop().sock_connect(self._socket, '/tmp/hivexplore/socket.sock')
                        print('Connected to ARGoS')
                        timeout_s = config.BASE_CONNECTION_TIMEOUT_S
                        break
                    except (FileNotFoundError, ConnectionRefusedError, socket.timeout) as exc:
                        print('UnixSocketServer connection error:', exc)
                        print(f'Connection to ARGoS failed, retrying after {timeout_s} seconds')
                        await asyncio.sleep(timeout_s)
                        timeout_s = min(timeout_s * 2, config.MAX_CONNECTION_TIMEOUT_S)

                # Start send and receive handler tasks
                tasks = [asyncio.create_task(task) for task in [self._receive_handler(), self._send_handler()]]

                try:
                    await asyncio.gather(*tasks)
                except (UnixSocketError, ConnectionResetError) as exc:
                    print('UnixSocketServer communication error:', exc)
                    for task in tasks:
                        task.cancel()
                    self._socket.close()
                    self._create_socket()
        finally:
            self._socket.close()

    def send(self, param_name: str, drone_id: str, value: Any):
        self._message_queue.put_nowait({
            'paramName': param_name,
            'droneId': drone_id,
            'value': value,
        })

    def bind(self, log_name: str, callback: Callable[[str, Any], None]):
        self._callbacks.setdefault(log_name, []).append(callback)

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
                for callback in self._callbacks.get(message['logName'], []):
                    callback(message['droneId'], message['value'])
            except (json.JSONDecodeError, KeyError) as exc:
                print('UnixSocketServer error: Invalid message received:', exc)

    async def _send_handler(self):
        while True:
            message = await self._message_queue.get()
            message_str = json.dumps(message)
            sent = await asyncio.get_event_loop().sock_sendall(self._socket, message_str.encode('utf-8'))

            if sent == 0:
                raise UnixSocketError('Socket connection broken in send handler')
