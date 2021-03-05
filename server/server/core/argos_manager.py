import math
from typing import Any, Optional, Set
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator, Orientation, Point
from server.core.unix_socket_client import UnixSocketClient

# pylint: disable=no-self-use

# TODO: Refactor common code between CrazyflieManager and ArgosManager


class ArgosManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
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
        self._web_socket_server.bind('set-led', self._set_led_enabled)

        await self._unix_socket_client.serve()

    # ARGoS callbacks

    def _get_drone_ids_callback(self, _drone_id: Optional[str], data: Any):
        self._drone_ids = data
        self._web_socket_server.send_message('drone-ids', list(self._drone_ids))

        print('Received drone IDs:', self._drone_ids)

    def _log_battery_callback(self, drone_id, data):
        battery_level = data['pm.batteryLevel']
        print(f'BatteryLevel from drone {drone_id}: {battery_level}')

        self._web_socket_server.send_drone_message('battery-level', drone_id, battery_level)

    def _log_orientation_callback(self, drone_id, data):
        measurements = {
            'roll': data['stateEstimate.roll'],
            'pitch': data['stateEstimate.pitch'],
            'yaw': data['stateEstimate.yaw'],
        }
        print(f'Orientation from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value:.2f}')

        self._map_generator.set_orientation(drone_id, Orientation(**measurements))

    def _log_position_callback(self, drone_id, data):
        measurements = {
            'x': data['stateEstimate.x'],
            'y': data['stateEstimate.y'],
            'z': data['stateEstimate.z'],
        }
        print(f'Position from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

        self._map_generator.set_position(drone_id, Point(**measurements))

    def _log_velocity_callback(self, drone_id, data):
        measurements = {
            'vx': data['stateEstimate.vx'],
            'vy': data['stateEstimate.vy'],
            'vz': data['stateEstimate.vz'],
        }
        print(f'Velocity from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

        velocity_magnitude = math.sqrt(measurements['vx']**2 + measurements['vy']**2 + measurements['vz']**2)
        print(f'Velocity magnitude: {velocity_magnitude}')

        self._web_socket_server.send_drone_message('velocity', drone_id, round(velocity_magnitude, 4))

    def _log_range_callback(self, drone_id, data):
        measurements = {
            'front': data['range.front'],
            'left': data['range.left'],
            'back': data['range.back'],
            'right': data['range.right'],
            'up': data['range.up'],
            'zrange': data['range.zrange'],
        }
        print(f'Range from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value}')

        self._map_generator.add_range_reading(drone_id, measurements)

    def _log_rssi_callback(self, drone_id, data):
        rssi = data['radio.rssi']
        print(f'RSSI from drone {drone_id}: {rssi}')

    # Client callbacks

    def _new_connection_callback(self, client_id):
        self._web_socket_server.send_message_to_client(client_id, 'drone-ids', list(self._drone_ids))

    def _set_led_enabled(self, drone_id: str, is_enabled: bool):
        if drone_id in self._drone_ids:
            print(f'Set LED state for drone {drone_id}: {is_enabled}')
            self._unix_socket_client.send('hivexplore.isM1LedOn', drone_id, is_enabled)
            self._web_socket_server.send_drone_message('set-led', drone_id, is_enabled)
        else:
            print('ArgosManager error: Unknown drone ID received:', drone_id)
