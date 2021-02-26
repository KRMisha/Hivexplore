import asyncio
import logging
from server.core.crazyflie_manager import CrazyflieManager
from server.core.argos_manager import ArgosManager
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)


class Server:
    def __init__(self, enable_debug_driver, is_argos_simulation):
        self._is_argos_simulation = is_argos_simulation
        self._map_generator = MapGenerator()
        self._web_socket_server = WebSocketServer()
        self._crazyflie_manager = CrazyflieManager(self._web_socket_server, self._map_generator,
                                                   enable_debug_driver) if not is_argos_simulation else None
        self._argos_manager = ArgosManager(self._web_socket_server, self._map_generator) if is_argos_simulation else None

    def start(self):
        if self._is_argos_simulation:
            asyncio.run(self._start_tasks_argos())
        else:
            asyncio.run(self._start_tasks_crazyflie())

    async def _start_tasks_argos(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._argos_manager.start(),
        )

    async def _start_tasks_crazyflie(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._crazyflie_manager.start(),
        )
