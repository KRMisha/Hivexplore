import logging
from threading import Timer
from cflib.crazyflie import Crazyflie
import log

# Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)

# This was inspired by the basiclog example
# Source: https://github.com/bitcraze/crazyflie-lib-python/blob/master/examples/basiclog.py

TIMER = 10


class ConnectionCrazyflie:
    def __init__(self, link_uri):
        self._crazyflie = Crazyflie(rw_cache='./cache')
        self._log = log.Log(self._crazyflie)
        self.is_connected = False

        self._crazyflie.connected.add_callback(self._connected)
        self._crazyflie.disconnected.add_callback(self._disconnected)
        self._crazyflie.connection_failed.add_callback(self._connection_failed)
        self._crazyflie.connection_lost.add_callback(self._connection_lost)

        print(f'Connection to {str(link_uri)}')

        self._crazyflie.open_link(link_uri)

    def _connected(self, link_uri):
        print(f'Connected to {str(link_uri)}')
        self.is_connected = True

        self._log.start_logging()

        timer = Timer(TIMER, self._crazyflie.close_link)
        timer.start()

    def _disconnected(self, link_uri):
        print(f'Disconnected from {str(link_uri)}')
        self.is_connected = False

    def _connection_failed(self, link_uri, msg):
        print(f'Connection to {str(link_uri)} failed: {str(msg)}')
        self.is_connected = False

    def _connection_lost(self, link_uri, msg):
        print(f'Connection to {str(link_uri)} lost: {str(msg)}')
        self.is_connected = False
