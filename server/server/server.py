import asyncio
import logging
from server.logger import Logger
from server.managers.crazyflie_manager import CrazyflieManager
from server.managers.argos_manager import ArgosManager
from server.map_generator import MapGenerator
from server.sockets.web_socket_server import WebSocketServer

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)


class Server:
    def __init__(self, is_argos_simulation, enable_debug_driver=False):
        self._is_argos_simulation = is_argos_simulation
        self._web_socket_server = WebSocketServer()
        self._logger = Logger(self._web_socket_server)
        self._map_generator = MapGenerator(self._web_socket_server, self._logger)
        self._drone_manager = CrazyflieManager(self._web_socket_server, self._logger, self._map_generator,
                                               enable_debug_driver) if not is_argos_simulation else ArgosManager(
                                                   self._web_socket_server, self._logger, self._map_generator)

    def start(self):
        asyncio.run(self._start_tasks())

    async def _start_tasks(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._drone_manager.start(),
        )
