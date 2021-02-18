import sys
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.core.socket_server import SocketServer


class CrazyflieManager:
    def __init__(self, socket_server: SocketServer, enable_debug_driver: bool):
        self._socket_server = socket_server
        self._crazyflies = {}
        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    def start(self):
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
            # TODO: Decide on appropriate handling
            # (Loop until connected? Should we keep trying to detect new connections?)
            print('No Crazyflies found, cannot control hive')
            sys.exit(1)

    # Param assigning methods

    def set_led_enabled(self, is_enabled: bool):
        for crazyflie in self._crazyflies.values():
            crazyflie.param.set_value('hivexplore.isM1LedOn', str(int(is_enabled)))

    # Setup

    def _setup_log(self, crazyflie: Crazyflie):
        configs = [
            {
                'log_config': LogConfig(name='BatteryLevel', period_in_ms=1000),
                'variables': ['pm.vbat'],
                'data_callback': self._log_battery_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Stabilizer', period_in_ms=1000),
                'variables': ['stabilizer.roll', 'stabilizer.pitch', 'stabilizer.yaw'],
                'data_callback': self._log_stabilizer_callback,
                'error_callback': self._log_error_callback,
            },
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
        # TODO: Store param group, name, and callback in a member variable
        crazyflie.param.add_update_callback(group='hivexplore', name='isM1LedOn', cb=self._param_update_callback)

    # Connection callbacks

    def _connected(self, link_uri):
        print(f'Connected to {link_uri}')

        self._setup_log(self._crazyflies[link_uri])
        self._setup_param(self._crazyflies[link_uri])

    def _disconnected(self, link_uri):
        print(f'Disconnected from {link_uri}')

    def _connection_failed(self, link_uri, msg):
        print(f'Connection to {link_uri} failed: {msg}')

    def _connection_lost(self, link_uri, msg):
        print(f'Connection to {link_uri} lost: {msg}')

    # Log callbacks

    def _log_battery_callback(self, timestamp, data, logconf):
        MIN_VOLTAGE = 3.0
        MAX_VOLTAGE = 4.23
        MAX_BATTERY_LEVEL = 100
        battery_level = (data['pm.vbat'] - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * MAX_BATTERY_LEVEL
        clamped_battery_level = max(0, min(battery_level, MAX_BATTERY_LEVEL))

        print(f'Battery level: {clamped_battery_level:.2f}')

        # TODO: Replace this with a proper message object
        self._socket_server.send(str(clamped_battery_level))

    def _log_stabilizer_callback(self, timestamp, data, logconf):
        print(f'{logconf.name}')
        print(f'- Roll: {data["stabilizer.roll"]:.2f}')
        print(f'- Pitch: {data["stabilizer.pitch"]:.2f}')
        print(f'- Yaw: {data["stabilizer.yaw"]:.2f}')

    def _log_error_callback(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    def _param_update_callback(self, name, value):
        print(f'Readback: {name}={value}')
