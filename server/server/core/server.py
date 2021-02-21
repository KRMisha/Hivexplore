import asyncio
import logging
from server.core.crazyflie_manager import CrazyflieManager
from server.core.web_socket_server import WebSocketServer
from server.core.unix_socket_server import UnixSocketServer
from server.core.map_generator import MapGenerator

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)


class Server:
    def __init__(self, enable_debug_driver):
        self._web_socket_server = WebSocketServer()
        self._unix_socket_server = UnixSocketServer()
        self._map_generator = MapGenerator()
        self._crazyflie_manager = CrazyflieManager(self._web_socket_server, enable_debug_driver)

    def start(self):
        asyncio.run(self._start_tasks())

    # TODO: Remove
    async def _test(self):
        while True:
            await asyncio.sleep(1)
            self._unix_socket_server.send('test', 'test')

    async def _start_tasks(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._unix_socket_server.serve(),
            self._crazyflie_manager.start(),
            self._test(),
        )
