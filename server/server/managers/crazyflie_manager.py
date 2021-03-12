import asyncio
from typing import Dict, List
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.managers.drone_manager import DroneManager
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator
from server import config


class CrazyflieManager(DroneManager):
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator, enable_debug_driver: bool):
        super().__init__(web_socket_server, map_generator)
        self._crazyflies: Dict[str, Crazyflie] = {}
        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    async def start(self):
        await self._find_crazyflies()
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

    def _get_drone_ids(self) -> List[str]:
        return list(self._crazyflies.keys())

    def _is_drone_id_valid(self, drone_id: str) -> bool:
        return drone_id in self._crazyflies

    def _set_drone_param(self, param: str, drone_id: str, value: str):
        self._crazyflies[drone_id].param.set_value(param, value)

    # Setup

    def _setup_log(self, crazyflie: Crazyflie):
        # Log config setup with the logged variables and success/error logging callbacks
        POLLING_PERIOD_MS = 1000

        log_configs = [
            {
                'log_config': LogConfig(name='BatteryLevel', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['pm.batteryLevel'],
                'data_callback': lambda _timestamp, data, logconf: self._log_battery_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Orientation', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.roll', 'stateEstimate.pitch', 'stateEstimate.yaw'],
                'data_callback': lambda _timestamp, data, logconf: self._log_orientation_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Position', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z'],
                'data_callback': lambda _timestamp, data, logconf: self._log_position_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Velocity', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.vx', 'stateEstimate.vy', 'stateEstimate.vz'],
                'data_callback': lambda _timestamp, data, logconf: self._log_velocity_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Range', period_in_ms=POLLING_PERIOD_MS), # Must be added after orientation and position
                'variables': ['range.front', 'range.left', 'range.back', 'range.right', 'range.up', 'range.zrange'],
                'data_callback': lambda _timestamp, data, logconf: self._log_range_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='Rssi', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['radio.rssi'],
                'data_callback': lambda _timestamp, data, logconf: DroneManager._log_rssi_callback(logconf.cf.link_uri, data),
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

    @staticmethod
    def _setup_param(crazyflie: Crazyflie):
        crazyflie.param.add_update_callback(group='hivexplore', name='missionState', cb=CrazyflieManager._param_update_callback)
        crazyflie.param.add_update_callback(group='hivexplore', name='isM1LedOn', cb=CrazyflieManager._param_update_callback)

    # Connection callbacks

    def _connected(self, link_uri):
        print(f'Connected to {link_uri}')
        self._setup_log(self._crazyflies[link_uri])
        CrazyflieManager._setup_param(self._crazyflies[link_uri])
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

    @staticmethod
    def _log_error_callback(logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    @staticmethod
    def _param_update_callback(name, value):
        print(f'Param readback: {name}={value}')
