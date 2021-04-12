import argparse
import asyncio
from typing import Union
from server.communication.web_socket_server import WebSocketServer
from server.logger import Logger
from server.managers.crazyflie_manager import CrazyflieManager
from server.managers.argos_manager import ArgosManager
from server.map_generator import MapGenerator


class Server:
    def __init__(self, args: argparse.Namespace):
        self._logger = Logger()
        self._web_socket_server = WebSocketServer(self._logger)
        self._logger.set_web_socket_server(self._web_socket_server)
        self._map_generator = MapGenerator(self._web_socket_server, self._logger)

        self._drone_manager: Union[CrazyflieManager, ArgosManager]
        if args.mode == 'drone':
            self._drone_manager = CrazyflieManager(self._web_socket_server, self._logger, self._map_generator, args.debug)
        elif args.mode == 'argos':
            self._drone_manager = ArgosManager(self._web_socket_server, self._logger, self._map_generator)

    def start(self):
        asyncio.run(self._start_tasks())

    async def _start_tasks(self):
        await asyncio.gather(
            self._web_socket_server.serve(),
            self._drone_manager.start(),
        )
