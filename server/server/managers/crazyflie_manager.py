import asyncio
import logging
from typing import Any, Dict, List
import cflib
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from server.logger import Logger
from server.managers.drone_manager import DroneManager
from server.managers.mission_state import MissionState
from server.map_generator import MapGenerator
from server.sockets.web_socket_server import WebSocketServer
from server.utils.uri import load_crazyflie_uris_from_file
from server.sockets.log_name_event import LogNameEvent
from server.sockets.param_name_event import ParamNameEvent


class CrazyflieManager(DroneManager):
    def __init__(self, web_socket_server: WebSocketServer, logger: Logger, map_generator: MapGenerator, enable_debug_driver: bool):
        super().__init__(web_socket_server, logger, map_generator)
        self._connected_crazyflies: Dict[str, Crazyflie] = {}
        self._pending_crazyflies: Dict[str, Crazyflie] = {}
        self._crazyflie_uris: List[str] = []

        try:
            self._crazyflie_uris = load_crazyflie_uris_from_file()
        except ValueError:
            self._logger.log_server_data(logging.WARN, 'CrazyflieManager warning: Could not load URIs from file')

        cflib.crtp.init_drivers(enable_debug_driver=enable_debug_driver)

    async def start(self):
        while True:
            if self._mission_state == MissionState.Standby:
                self._connect_crazyflies()

            CRAZYFLIE_CONNECTION_PERIOD_S = 5
            await asyncio.sleep(CRAZYFLIE_CONNECTION_PERIOD_S)

    def _connect_crazyflies(self):
        for uri in self._crazyflie_uris:
            if uri in self._connected_crazyflies:
                continue

            # If a Crazyflie is still pending, force close its connection
            if uri in self._pending_crazyflies:
                self._logger.log_server_data(logging.WARN, f'CrazyflieManager warning: Force disconnecting pending drone: {uri}')
                self._pending_crazyflies[uri].close_link()

            self._logger.log_server_data(logging.INFO, f'Trying to connect to: {uri}')
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
        super()._set_drone_param(param, drone_id, value)
        self._connected_crazyflies[drone_id].param.set_value(param, value)

    # Setup

    def _setup_log(self, crazyflie: Crazyflie):
        # Log config setup with the logged variables and success/error logging callbacks
        POLLING_PERIOD_MS = 1000

        log_configs = [
            {
                'log_config': LogConfig(name=LogNameEvent.BatteryLevel.value, period_in_ms=POLLING_PERIOD_MS),
                'variables': ['pm.batteryLevel'],
                'data_callback': lambda _timestamp, data, logconf: self._log_battery_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.Orientation.value, period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.roll', 'stateEstimate.pitch', 'stateEstimate.yaw'],
                'data_callback': lambda _timestamp, data, logconf: self._log_orientation_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.Position.value, period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z'],
                'data_callback': lambda _timestamp, data, logconf: self._log_position_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.Velocity.value, period_in_ms=POLLING_PERIOD_MS),
                'variables': ['stateEstimate.vx', 'stateEstimate.vy', 'stateEstimate.vz'],
                'data_callback': lambda _timestamp, data, logconf: self._log_velocity_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.Range.value,
                                        period_in_ms=POLLING_PERIOD_MS), # Must be added after orientation and position
                'variables': ['range.front', 'range.left', 'range.back', 'range.right', 'range.up', 'range.zrange'],
                'data_callback': lambda _timestamp, data, logconf: self._log_range_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.Rssi.value, period_in_ms=POLLING_PERIOD_MS),
                'variables': ['radio.rssi'],
                'data_callback': lambda _timestamp, data, logconf: self._log_rssi_callback(logconf.cf.link_uri, data),
                'error_callback': self._log_error_callback,
            },
            {
                'log_config': LogConfig(name=LogNameEvent.DroneStatus.value, period_in_ms=POLLING_PERIOD_MS),
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
                self._logger.log_server_data(
                    logging.ERROR, f'CrazyflieManager error: Could not start logging data, {exc} was not found in the Crazyflie TOC')
            except AttributeError as exc:
                self._logger.log_server_data(logging.ERROR, f'CrazyflieManager error: Could not add log configuration: {exc}')

    def _setup_param(self, crazyflie: Crazyflie):
        crazyflie.param.add_update_callback(group='hivexplore', name=ParamNameEvent.MissionState.value, cb=self._param_update_callback)
        crazyflie.param.add_update_callback(group='hivexplore', name=ParamNameEvent.IsM1LedOn.value, cb=self._param_update_callback)

    # Connection callbacks

    def _connected(self, link_uri: str):
        self._logger.log_server_data(logging.INFO, f'Connected to {link_uri}')
        self._logger.log_drone_data(logging.INFO, link_uri, 'Connected')

        if self._mission_state != MissionState.Standby:
            self._logger.log_server_data(logging.WARN, f'CrazyflieManager warning: Ignoring drone connection during mission: {link_uri}')
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
        self._logger.log_server_data(logging.INFO, f'Disconnected from {link_uri}')
        self._logger.log_drone_data(logging.INFO, link_uri, 'Disconnected')
        self._connected_crazyflies.pop(link_uri, None)
        self._pending_crazyflies.pop(link_uri, None) # Pop in case a drone gets disconnected while attempting to connect
        self._send_drone_ids()

        self._drone_statuses.pop(link_uri, None)
        self._drone_leds.pop(link_uri, None)
        self._drone_battery_levels.pop(link_uri, None)

    def _connection_failed(self, link_uri: str, msg: str):
        self._logger.log_server_data(logging.WARN, f'Connection to {link_uri} failed: {msg}')
        self._logger.log_drone_data(logging.WARN, link_uri, 'Connection failed')
        del self._pending_crazyflies[link_uri]

    def _connection_lost(self, link_uri: str, msg: str):
        self._logger.log_server_data(logging.INFO, f'Connection to {link_uri} lost: {msg}')
        self._logger.log_drone_data(logging.INFO, link_uri, 'Connection lost')
        self._connected_crazyflies.pop(link_uri, None) # Avoid double delete when Crazyflie disconnects

    # Log callbacks

    def _log_error_callback(self, logconf, msg):
        self._logger.log_server_data(logging.ERROR, f'Error when logging {logconf.name}: {msg}')

    # Param callbacks

    def _param_update_callback(self, name, value):
        self._logger.log_server_data(logging.INFO, f'Param readback: {name}={value}')
