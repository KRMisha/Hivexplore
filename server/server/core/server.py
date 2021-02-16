import time
import logging
from server.core.crazyflie_manager import CrazyflieManager
from server.core.socket_server import SocketServer
from server.core.map_generator import MapGenerator

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR) # TODO: Investigate more liberal logging levels


class Server:
    def __init__(self, enable_debug_driver):
        self._socket_server = SocketServer()
        self._map_generator = MapGenerator()
        self._crazyflie_manager = CrazyflieManager(self._socket_server, enable_debug_driver)

    def start(self):
        self._crazyflie_manager.start()
        self._socket_server.serve()

        while True:
            time.sleep(1)
            # TODO: Update map every second
            # self._map_generator.update()

    # TODO: Communication callbacks between the managers
