import logging
from cflib.crazyflie import Crazyflie
from server.logger import Logger
from server.param_manager import ParamManager

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)

# This was inspired by the basiclog example
# Source: https://github.com/bitcraze/crazyflie-lib-python/blob/master/examples/basiclog.py


class CrazyflieConnection:
    def __init__(self, link_uri):
        self._crazyflie = Crazyflie(rw_cache='./cache')

        self._logger = Logger(self._crazyflie)
        self._param_manager = ParamManager(self._crazyflie)

        self.is_connected = False

        self._crazyflie.connected.add_callback(self._connected)
        self._crazyflie.disconnected.add_callback(self._disconnected)
        self._crazyflie.connection_failed.add_callback(self._connection_failed)
        self._crazyflie.connection_lost.add_callback(self._connection_lost)

        print(f'Connecting to {link_uri}')
        self._crazyflie.open_link(link_uri)

    def _connected(self, link_uri):
        print(f'Connected to {link_uri}')
        self.is_connected = True
        self._logger.start_logging()

    def _disconnected(self, link_uri):
        print(f'Disconnected from {link_uri}')
        self.is_connected = False

    def _connection_failed(self, link_uri, msg):
        print(f'Connection to {link_uri} failed: {msg}')
        self.is_connected = False

    def _connection_lost(self, link_uri, msg):
        print(f'Connection to {link_uri} lost: {msg}')
        self.is_connected = False
