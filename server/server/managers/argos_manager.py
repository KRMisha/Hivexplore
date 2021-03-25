from typing import Any, Optional, Set, List
from server.logger import Logger
from server.managers.drone_manager import DroneManager
from server.map_generator import MapGenerator
from server.sockets.unix_socket_client import UnixSocketClient
from server.sockets.web_socket_server import WebSocketServer


class ArgosManager(DroneManager):
    def __init__(self, web_socket_server: WebSocketServer, logger: Logger, map_generator: MapGenerator):
        super().__init__(web_socket_server, logger, map_generator)
        self._unix_socket_client = UnixSocketClient(logger)
        self._drone_ids: Set[str] = set()

    async def start(self):
        # ARGoS bindings
        self._unix_socket_client.bind('disconnect', self._unix_socket_disconnect_callback)
        self._unix_socket_client.bind('drone-ids', self._get_drone_ids_callback)
        self._unix_socket_client.bind('BatteryLevel', self._log_battery_callback)
        self._unix_socket_client.bind('Orientation', self._log_orientation_callback)
        self._unix_socket_client.bind('Position', self._log_position_callback)
        self._unix_socket_client.bind('Velocity', self._log_velocity_callback)
        self._unix_socket_client.bind('Range', self._log_range_callback)
        self._unix_socket_client.bind('Rssi', self._log_rssi_callback)
        self._unix_socket_client.bind('DroneStatus', self._log_drone_status_callback)
        self._unix_socket_client.bind('Console', self._log_console_callback)

        await self._unix_socket_client.serve()

    def _get_drone_ids(self) -> List[str]:
        return list(self._drone_ids)

    def _is_drone_id_valid(self, drone_id: str) -> bool:
        return drone_id in self._drone_ids

    def _set_drone_param(self, param: str, drone_id: str, value: Any):
        super()._set_drone_param(param, drone_id, value)
        self._unix_socket_client.send(param, drone_id, value)

    def _unix_socket_disconnect_callback(self):
        self._drone_ids = []
        self._send_drone_ids()

    def _get_drone_ids_callback(self, _drone_id: Optional[str], data: Any):
        self._drone_ids = data
        self._send_drone_ids()

        self._logger.log_server_data(f'Received drone IDs: {self._drone_ids}')

    def _log_console_callback(self, drone_id: str, data: str):
        for line in data.split('\n'):
            super()._log_console_callback(drone_id, line)
