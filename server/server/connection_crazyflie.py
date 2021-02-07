import logging
from threading import Timer
from cflib.crazyflie import Crazyflie
import log

#Only output errors from the logging framework
logging.basicConfig(level=logging.ERROR)

#This was inspired by the basiclog example
#Source: https://github.com/bitcraze/crazyflie-lib-python/blob/master/examples/basiclog.py

TIMER = 10

class ConnectionCrazyflie:

    def __init__(self, link_uri):
        self._cf = Crazyflie(rw_cache='./cache')
        self._log = log.Log(self._cf)
        self.is_connected = False

        self._cf.connected.add_callback(self._connected)
        self._cf.disconnected.add_callback(self._disconnected)
        self._cf.connection_failed.add_callback(self._connection_failed)
        self._cf.connection_lost.add_callback(self._connection_lost)

        print('Connection to %s' % link_uri)

        self._cf.open_link(link_uri)

    def _connected(self, link_uri):
        print('Connected to %s' % link_uri)
        self.is_connected = True
        try:
            log.start_logging(self._cf)
        #change print messages
        except KeyError:
            print('Log configuration error')
        except AttributeError:
            print('Bad configuration')

        timer = Timer(TIMER, self._cf.close_link)
        timer.start()

    def _disconnected(self, link_uri):
        print('Disconnected from %s' % link_uri)
        self.is_connected = False

    def _connection_failed(self, link_uri, msg):
        print('Connection to %s failed: %s' % (link_uri, msg))
        self.is_connected = False

    def _connection_lost(self, link_uri, msg):
        print('Connection to %s lost: %s' %(link_uri, msg))
        self.is_connected = False
