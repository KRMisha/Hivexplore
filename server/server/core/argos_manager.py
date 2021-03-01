from typing import Any, Optional
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator
from server.core.unix_socket_client import UnixSocketClient


class ArgosManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
        self._unix_socket_client = UnixSocketClient()
        self._drone_ids = set()

    async def start(self):
        # Client bindings
        self._web_socket_server.bind('connect', self._new_connection_callback)
        self._web_socket_server.bind('set-led', self._set_led_enabled)

        # ARGoS bindings
        self._unix_socket_client.bind('drone-ids', self._get_drone_ids_callback)
        self._unix_socket_client.bind('pm.batteryLevel', self._log_battery_callback)
        # TODO: Handle ['stabilizer.roll', 'stabilizer.pitch', 'stabilizer.yaw']
        # TODO: Handle ['range.front', 'range.back', 'range.up', 'range.left', 'range.right', 'range.zrange']
        # TODO: Handle ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z']

        await self._unix_socket_client.serve()

    # Client callbacks

    def _new_connection_callback(self, client_id):
        self._web_socket_server.send_message_to_client(client_id, 'drone-ids', self._drone_ids)

    def _set_led_enabled(self, drone_id: str, is_enabled: bool):
        if drone_id in self._drone_ids:
            self._unix_socket_client.send_drone_message('hivexplore.isM1LedOn', drone_id, is_enabled)
            self._web_socket_server.send_drone_message('set-led', drone_id, is_enabled)
        else:
            print('ArgosManager error: Unknown drone ID received:', drone_id)

    # ARGoS callbacks

    def _get_drone_ids_callback(self, _drone_id: Optional[str], value: Any):
        self._drone_ids = value
        self._web_socket_server.send_message('drone-ids', self._drone_ids)

        print('Received drone IDs:', self._drone_ids)

    def _log_battery_callback(self, drone_id: str, value: Any):
        self._web_socket_server.send_drone_message('battery-level', drone_id, value)
        print(f'Received battery level from drone {drone_id}: {value}')
