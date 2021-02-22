import time
from typing import Dict
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.core.socket_server import SocketServer
from server.core.map_generator import MapGenerator

# pylint: disable=no-self-use


class CrazyflieManager:
    def __init__(self, socket_server: SocketServer, map_generator: MapGenerator, enable_debug_driver: bool):
        self._socket_server = socket_server
        self._map_generator = map_generator
        self._crazyflies: Dict[str, Crazyflie] = {}
        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    def start(self):
        self._find_crazyflies()
        self._socket_server.bind('set-led', self._set_led_enabled)

    def _find_crazyflies(self):
        while len(self._crazyflies) == 0:
            print('Scanning interfaces for Crazyflies...')
            available_interfaces = cflib.crtp.scan_interfaces()

            print('Crazyflies found:')
            for available_interface in available_interfaces:
                print(available_interface[0])

            if len(available_interfaces) > 0:
                for available_interface in available_interfaces:
                    crazyflie = Crazyflie(rw_cache='./cache')

                    crazyflie.connected.add_callback(self._connected)
                    crazyflie.disconnected.add_callback(self._disconnected)
                    crazyflie.connection_failed.add_callback(self._connection_failed)
                    crazyflie.connection_lost.add_callback(self._connection_lost)

                    link_uri = available_interface[0]
                    print(f'Connecting to {link_uri}')
                    crazyflie.open_link(link_uri)

                    self._crazyflies[link_uri] = crazyflie
            else:
                print('No Crazyflies found, retrying after 5 seconds')
                time.sleep(5)

    # Setup

    def _setup_log(self, crazyflie: Crazyflie):
        # Log config setup with the logged variables and success/error logging callbacks
        POLLING_PERIOD_MS = 1000
        configs = [
            {
                'log_config': LogConfig(name='BatteryLevel', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['pm.batteryLevel'],
                'data_callback': self._log_battery_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Stabilizer', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stabilizer.roll', 'stabilizer.pitch', 'stabilizer.yaw'],
                'data_callback': self._log_stabilizer_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Range', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['range.front', 'range.back', 'range.up', 'range.left', 'range.right', 'range.zrange'],
                'data_callback': self._log_range_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Position', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z'],
                'data_callback': self._log_position_callback,
                'error_callback': self._log_error_callback,
            }
        ]

        for config in configs:
            try:
                for variable in config['variables']:
                    config['log_config'].add_variable(variable)

                crazyflie.log.add_config(config['log_config'])
                config['log_config'].data_received_cb.add_callback(config['data_callback'])
                config['log_config'].error_cb.add_callback(config['error_callback'])
                config['log_config'].start()
            except KeyError as exc:
                print(f'Could not start logging data, {exc} was not found in the Crazyflie TOC')
            except AttributeError as exc:
                print(f'Could not add log configuration, error: {exc}')

    def _setup_param(self, crazyflie: Crazyflie):
        crazyflie.param.add_update_callback(group='hivexplore', name='isM1LedOn', cb=self._param_update_callback)

    # Connection callbacks

    def _connected(self, link_uri):
        print(f'Connected to {link_uri}')

        self._setup_log(self._crazyflies[link_uri])
        self._setup_param(self._crazyflies[link_uri])

    def _disconnected(self, link_uri):
        print(f'Disconnected from {link_uri}')
        del self._crazyflies[link_uri]

    def _connection_failed(self, link_uri, msg):
        print(f'Connection to {link_uri} failed: {msg}')
        del self._crazyflies[link_uri]

    def _connection_lost(self, link_uri, msg):
        print(f'Connection to {link_uri} lost: {msg}')
        self._crazyflies.pop(link_uri, None) # Avoid double delete when Crazyflie disconnects

    # Log callbacks

    def _log_battery_callback(self, _timestamp, data, _logconf):
        battery_level = data["pm.batteryLevel"]
        print(f'Battery level: {battery_level}')
        # TODO: Retrieve drone ID and send it to the client
        drone_id = 0
        self._socket_server.send('battery-level', drone_id, battery_level)

    def _log_stabilizer_callback(self, _timestamp, data, logconf):
        measurements = {
            'roll': data["stabilize.roll"],
            'pitch': pitch = data["stabilize.pitch"],
            'yaw': data["stabilize.yaw"],
        }

        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.2f}')

        # TODO: Retrieve drone ID and send it to the client
        drone_id = 0
        self._socket_server.send('stabilizer-data', drone_id, measurements)

    def _log_range_callback(self, _timestamp, data, logconf):
        measurements = {
            'front': data["range.front"],
            'back': data["range.back"],
            'up': data["range.up"],
            'left': data["range.left"],
            'right': data["range.right"],
            'zrange': data["range.zrange"]
        }
        self._map_generator.add_points(measurements)
        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.2f}')

        # TODO: Retrieve drone ID and send it to the client
        drone_id = 0
        self._socket_server.send('range-data', drone_id, measurements)

    def _log_position_callback(self, _timestamp, data, logconf):
        measurements = {
           'x': data["stateEstimate.x"],
           'y': data["stateEstimate.y"],
           'z': data["stateEstimate.z"]
        }
        self._map_generator.add_position(measurements)
        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

        # TODO: Retrieve drone ID and send it to the client
        drone_id = 0
        self._socket_server.send('position-data', drone_id, measurements)

    def _log_error_callback(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    def _param_update_callback(self, name, value):
        print(f'Readback: {name}={value}')

    # Param assigning methods

    def _set_led_enabled(self, is_enabled: bool):
        for crazyflie in self._crazyflies.values():
            crazyflie.param.set_value('hivexplore.isM1LedOn', is_enabled)
