import asyncio
from typing import Dict
import numpy as np
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator, Orientation, Point
from server import config

# pylint: disable=no-self-use

# TODO: Refactor common code between CrazyflieManager and ArgosManager


class CrazyflieManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator, enable_debug_driver: bool):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
        self._crazyflies: Dict[str, Crazyflie] = {}
        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    async def start(self):
        await self._find_crazyflies()
        self._web_socket_server.bind('connect', self._new_connection_callback)
        self._web_socket_server.bind('mission-state', self._set_mission_state)
        self._web_socket_server.bind('set-led', self._set_led_enabled)

    async def _find_crazyflies(self):
        timeout_s = config.BASE_CONNECTION_TIMEOUT_S
        while len(self._crazyflies) == 0:
            available_interfaces = await asyncio.get_event_loop().run_in_executor(None, cflib.crtp.scan_interfaces)

            if len(available_interfaces) > 0:
                print('Crazyflies found:')
                for available_interface in available_interfaces:
                    print(f'- {available_interface[0]}')

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
                timeout_s = config.BASE_CONNECTION_TIMEOUT_S
            else:
                print(f'No Crazyflies found, retrying after {timeout_s} seconds')
                await asyncio.sleep(timeout_s)
                timeout_s = min(timeout_s * 2, config.MAX_CONNECTION_TIMEOUT_S)

    def _send_drone_ids(self, client_id=None):
        if client_id is None:
            self._web_socket_server.send_message('drone-ids', list(self._crazyflies.keys()))
        else:
            self._web_socket_server.send_message_to_client(client_id, 'drone-ids', list(self._crazyflies.keys()))

    # Setup

    def _setup_log(self, crazyflie: Crazyflie):
        # Log config setup with the logged variables and success/error logging callbacks
        POLLING_PERIOD_MS = 1000

        log_configs = [
            {
                'log_config': LogConfig(name='BatteryLevel', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['pm.batteryLevel'],
                'data_callback': self._log_battery_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Orientation', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.roll', 'stateEstimate.pitch', 'stateEstimate.yaw'],
                'data_callback': self._log_orientation_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Position', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z'],
                'data_callback': self._log_position_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Velocity', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.vx', 'stateEstimate.vy', 'stateEstimate.vz'],
                'data_callback': self._log_velocity_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Range', period_in_ms=POLLING_PERIOD_MS), # Must be added after orientation and position
                'variables': ['range.front', 'range.left', 'range.back', 'range.right', 'range.up', 'range.zrange'],
                'data_callback': self._log_range_callback,
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Rssi', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['radio.rssi'],
                'data_callback': self._log_rssi_callback,
                'error_callback': self._log_error_callback,
            },
        ]

        for log_config in log_configs:
            try:
                for variable in log_config['variables']:
                    log_config['log_config'].add_variable(variable)

                crazyflie.log.add_config(log_config['log_config'])
                log_config['log_config'].data_received_cb.add_callback(log_config['data_callback'])
                log_config['log_config'].error_cb.add_callback(log_config['error_callback'])
                log_config['log_config'].start()
            except KeyError as exc:
                print(f'Could not start logging data, {exc} was not found in the Crazyflie TOC')
            except AttributeError as exc:
                print(f'Could not add log configuration, error: {exc}')

    def _setup_param(self, crazyflie: Crazyflie):
        crazyflie.param.add_update_callback(group='hivexplore', name='missionState', cb=self._param_update_callback)
        crazyflie.param.add_update_callback(group='hivexplore', name='isM1LedOn', cb=self._param_update_callback)

    # Connection callbacks

    def _connected(self, link_uri):
        print(f'Connected to {link_uri}')
        self._setup_log(self._crazyflies[link_uri])
        self._setup_param(self._crazyflies[link_uri])
        self._send_drone_ids()

    def _disconnected(self, link_uri):
        print(f'Disconnected from {link_uri}')
        del self._crazyflies[link_uri]
        self._send_drone_ids()

    def _connection_failed(self, link_uri, msg):
        print(f'Connection to {link_uri} failed: {msg}')
        del self._crazyflies[link_uri]

    def _connection_lost(self, link_uri, msg):
        print(f'Connection to {link_uri} lost: {msg}')
        self._crazyflies.pop(link_uri, None) # Avoid double delete when Crazyflie disconnects

    # Log callbacks

    def _log_battery_callback(self, _timestamp, data, logconf):
        battery_level = data['pm.batteryLevel']
        print(f'{logconf.name}: {battery_level}')

        self._web_socket_server.send_drone_message('battery-level', logconf.cf.link_uri, battery_level)

    def _log_orientation_callback(self, _timestamp, data, logconf):
        measurements = {
            'roll': data['stateEstimate.roll'],
            'pitch': data['stateEstimate.pitch'],
            'yaw': data['stateEstimate.yaw'],
        }
        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.2f}')

        self._map_generator.set_orientation(logconf.cf.link_uri, Orientation(**measurements))

    def _log_position_callback(self, _timestamp, data, logconf):
        measurements = {
            'x': data['stateEstimate.x'],
            'y': data['stateEstimate.y'],
            'z': data['stateEstimate.z'],
        }

        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

        self._map_generator.set_position(logconf.cf.link_uri, Point(**measurements))

    def _log_velocity_callback(self, _timestamp, data, logconf):
        measurements = {
            'vx': data['stateEstimate.vx'],
            'vy': data['stateEstimate.vy'],
            'vz': data['stateEstimate.vz'],
        }
        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

        velocity_magnitude = np.linalg.norm(list(measurements.values()))
        print(f'Velocity magnitude: {velocity_magnitude}')

        self._web_socket_server.send_drone_message('velocity', logconf.cf.link_uri, round(velocity_magnitude, 4))

    def _log_range_callback(self, _timestamp, data, logconf):
        measurements = {
            'front': data['range.front'],
            'left': data['range.left'],
            'back': data['range.back'],
            'right': data['range.right'],
            'up': data['range.up'],
            'zrange': data['range.zrange'],
        }
        print(logconf.name)
        for key, value in measurements.items():
            print(f'- {key}: {value}')

        self._map_generator.add_range_reading(logconf.cf.link_uri, measurements)

    def _log_rssi_callback(self, _timestamp, data, logconf):
        rssi = data['radio.rssi']
        print(f'{logconf.name}: {rssi}')

    def _log_error_callback(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    def _param_update_callback(self, name, value):
        print(f'Param readback: {name}={value}')

    # Client callbacks

    def _new_connection_callback(self, client_id):
        self._send_drone_ids(client_id)

    def _set_mission_state(self, mission_state: str):
        print('Set mission state:', mission_state)
        for crazyflie in self._crazyflies.values():
            # crazyflie.param.set_value('hivexplore.missionState', mission_state) # TODO: Check type to send
            pass
        self._web_socket_server.send_message('mission-state', mission_state)

    def _set_led_enabled(self, drone_id, is_enabled: bool):
        if drone_id in self._crazyflies:
            print(f'Set LED state for drone {drone_id}: {is_enabled}')
            self._crazyflies[drone_id].param.set_value('hivexplore.isM1LedOn', is_enabled)
            self._web_socket_server.send_drone_message('set-led', drone_id, is_enabled)
        else:
            print('CrazyflieManager error: Unknown drone ID received:', drone_id)
