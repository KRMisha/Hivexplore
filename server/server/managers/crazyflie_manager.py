from typing import Any, Dict, List
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.managers.drone_manager import DroneManager
from server.managers.mission_state import MissionState
from server.map_generator import MapGenerator
from server.sockets.web_socket_server import WebSocketServer
from server.uri_utils import load_crazyflie_uris_from_file


class CrazyflieManager(DroneManager):
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator, enable_debug_driver: bool):
        super().__init__(web_socket_server, map_generator)
        self._connected_crazyflies: Dict[str, Crazyflie] = {}
        self._pending_crazyflies: Dict[str, Crazyflie] = {}
        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    async def start(self):
        self._connect_crazyflies()
        self._web_socket_server.bind('mission-state', self._set_mission_state)
        self._web_socket_server.bind('set-led', self._set_led_enabled)

    def _connect_crazyflies(self):
        try:
            crazyflie_uris = load_crazyflie_uris_from_file()
        except ValueError:
            return

        for uri in crazyflie_uris:
            if uri in self._connected_crazyflies or uri in self._pending_crazyflies:
                continue

            print(f'Trying to connect to: {uri}')
            crazyflie = Crazyflie(rw_cache='./cache')

            crazyflie.connected.add_callback(self._connected)
            crazyflie.disconnected.add_callback(self._disconnected)
            crazyflie.connection_failed.add_callback(self._connection_failed)
            crazyflie.connection_lost.add_callback(self._connection_lost)

            crazyflie.open_link(uri)
            self._pending_crazyflies[uri] = crazyflie

    def _get_drone_ids(self) -> List[str]:
        return list(self._connected_crazyflies.keys())

    def _is_drone_id_valid(self, drone_id: str) -> bool:
        return drone_id in self._connected_crazyflies

    def _set_drone_param(self, param: str, drone_id: str, value: Any):
        self._connected_crazyflies[drone_id].param.set_value(param, value)

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
                'data_callback': lambda _timestamp, data, logconf: self._log_rssi_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name='DroneStatus', period_in_ms=POLLING_PERIOD_MS),
                'variables': ['hivexplore.droneStatus'],
                'data_callback': lambda _timestamp, data, logconf: self._log_drone_status_callback(logconf.cf.link_uri, data),
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

    def _connected(self, link_uri: str):
        print(f'Connected to {link_uri}')

        if self._mission_state != MissionState.STANDBY:
            print('CrazyflieManager warning: Ignoring drone connection during mission:', link_uri)
            self._pending_crazyflies[link_uri].close_link()
            return

        self._connected_crazyflies[link_uri] = self._pending_crazyflies[link_uri]
        del self._pending_crazyflies[link_uri]

        self._setup_log(self._connected_crazyflies[link_uri])
        self._setup_param(self._connected_crazyflies[link_uri])

        # Setup console logging
        self._connected_crazyflies[link_uri].console.receivedChar.add_callback(lambda data: self._log_console_callback(link_uri, data))

        self._send_drone_ids()

    def _disconnected(self, link_uri: str):
        print(f'Disconnected from {link_uri}')
        self._connected_crazyflies.pop(link_uri, None)
        self._pending_crazyflies.pop(link_uri, None)
        self._send_drone_ids()

    def _connection_failed(self, link_uri: str, msg: str):
        print(f'Connection to {link_uri} failed: {msg}')
        del self._pending_crazyflies[link_uri]

    def _connection_lost(self, link_uri: str, msg: str):
        print(f'Connection to {link_uri} lost: {msg}')
        self._connected_crazyflies.pop(link_uri, None) # Avoid double delete when Crazyflie disconnects

    # Log callbacks

    @staticmethod
    def _log_error_callback(logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    @staticmethod
    def _param_update_callback(name, value):
        print(f'Param readback: {name}={value}')
