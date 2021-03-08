from typing import Any, Optional, Set
from server.managers.drone_manager import DroneManager
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator
from server.core.unix_socket_client import UnixSocketClient
from server.managers.mission_state import MissionState

# pylint: disable=no-self-use


class ArgosManager(DroneManager):
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        DroneManager.__init__(self, web_socket_server, map_generator)
        self._unix_socket_client = UnixSocketClient()
        self._drone_ids: Set[str] = set()

    async def start(self):
        # ARGoS bindings
        self._unix_socket_client.bind('drone-ids', self._get_drone_ids_callback)
        self._unix_socket_client.bind('BatteryLevel', self._log_battery_callback)
        self._unix_socket_client.bind('Orientation', self._log_orientation_callback)
        self._unix_socket_client.bind('Position', self._log_position_callback)
        self._unix_socket_client.bind('Velocity', self._log_velocity_callback)
        self._unix_socket_client.bind('Range', self._log_range_callback)
        self._unix_socket_client.bind('Rssi', self._log_rssi_callback)

        # Client bindings
        self._web_socket_server.bind('connect', self._new_connection_callback)
        self._web_socket_server.bind('mission-state', self._set_mission_state)
        self._web_socket_server.bind('set-led', self._set_led_enabled)

        await self._unix_socket_client.serve()

    # ARGoS callbacks

    def _get_drone_ids(self):
        return list(self._drone_ids)

    def _get_drone_ids_callback(self, _drone_id: Optional[str], data: Any):
        self._drone_ids = data
        self._send_drone_ids()

        print('Received drone IDs:', self._drone_ids)

    # Client callbacks

    def _set_mission_state(self, mission_state_str: str):
        try:
            mission_state = MissionState[mission_state_str.upper()]
        except KeyError:
            print('ArgosManager error: Unknown mission state received:', mission_state_str)
            return

        print('Set mission state:', mission_state)
        for drone_id in self._drone_ids:
            self._unix_socket_client.send('hivexplore.missionState', drone_id, mission_state)
        self._web_socket_server.send_message('mission-state', mission_state_str)

    def _set_led_enabled(self, drone_id: str, is_enabled: bool):
        if drone_id in self._drone_ids:
            print(f'Set LED state for drone {drone_id}: {is_enabled}')
            self._unix_socket_client.send('hivexplore.isM1LedOn', drone_id, is_enabled)
            self._web_socket_server.send_drone_message('set-led', drone_id, is_enabled)
        else:
            print('ArgosManager error: Unknown drone ID received:', drone_id)
