import asyncio
import logging
from server.core.crazyflie_manager import CrazyflieManager
from server.core.argos_manager import ArgosManager
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)


class Server:
    def __init__(self, is_argos_simulation, enable_debug_driver=False):
        self._is_argos_simulation = True
        self._map_generator = MapGenerator()
        self._web_socket_server = WebSocketServer()
        self._crazyflie_manager = CrazyflieManager(self._web_socket_server, self._map_generator,
                                                   enable_debug_driver) if not is_argos_simulation else None
        self._argos_manager = ArgosManager(self._web_socket_server, self._map_generator) if is_argos_simulation else None

    def start(self):
        asyncio.run(self._start_tasks())

    async def _start_tasks(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._crazyflie_manager.start() if not self._is_argos_simulation else self._argos_manager.start(),
        )
