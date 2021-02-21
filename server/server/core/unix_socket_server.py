import asyncio
import json
import socket
from typing import Any, Callable, Dict, List


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

        timeout_s = 2
        try:
            # Main loop
            while True:
                # Connection attempt loop
                while True:
                    try:
                        await asyncio.get_event_loop().sock_connect(self._socket, '/tmp/hivexplore/socket.sock')
                        print('Connected to ARGoS')
                        timeout_s = 2
                        break
                    except (FileNotFoundError, ConnectionRefusedError, socket.timeout) as exc:
                        print('UnixSocketServer connection error:', exc)
                        print(f'Connection to ARGoS failed, retrying after {timeout_s} seconds')
                        await asyncio.sleep(timeout_s)
                        timeout_s *= 2

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

    def send(self, event: str, data: Any):
        self._message_queue.put_nowait({'event': event, 'data': data})

    def bind(self, event: str, callback: Callable[[Any], None]):
        self._callbacks.setdefault(event, []).append(callback)

    def _create_socket(self):
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        self._socket.setblocking(False)

    async def _receive_handler(self):
        while True:
            message_bytes = await asyncio.get_event_loop().sock_recv(self._socket, 4096)

            if len(message_bytes) == 0:
                raise UnixSocketError('Socket connection broken in receive handler')

            try:
                # TODO: Uncomment
                # message = json.loads(message_bytes.decode('utf-8'))
                message = message_bytes.decode('utf-8')
                print(message) # TODO: Remove
                # TODO: Uncomment
                # for callback in self._callbacks.get(message['event'], []):
                #     callback(message['data'])
            except (json.JSONDecodeError, KeyError) as exc:
                print('UnixSocketServer error: Invalid message received:', exc)

    async def _send_handler(self):
        while True:
            message = await self._message_queue.get()
            # message_str = json.dumps(message) # TODO: Uncomment
            sent = await asyncio.get_event_loop().sock_sendall(self._socket,
                                                               'bonjour\n'.encode('utf-8')) # TODO: Replace 'bonjour\n' with message_str

            if sent == 0:
                raise UnixSocketError('Socket connection broken in send handler')
