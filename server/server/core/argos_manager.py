from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator
from server.core.unix_socket_client import UnixSocketClient


class ArgosManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
        self._unix_socket_client = UnixSocketClient()

    async def start(self):
        # Client bindings
        self._web_socket_server.bind('set-led', self._set_led_enabled)

        # Argos bindings
        self._unix_socket_client.bind('BatteryLevel', self._log_battery_callback)
        self._unix_socket_client.bind('Orientation', self._log_orientation_callback)
        self._unix_socket_client.bind('Position', self._log_position_callback)
        self._unix_socket_client.bind('Range', self._log_range_callback)

        await self._unix_socket_client.serve()

    def _set_led_enabled(self, is_enabled: bool):
        # TODO: Get drone IDs on connection to ARGoS and loop for all drone IDs
        drone_id = 's0'
        self._unix_socket_client.send('hivexplore.isM1LedOn', drone_id, is_enabled)

    def _log_battery_callback(self, drone_id, data):
        battery_level = data['pm.batteryLevel']
        print(f'BatteryLevel from drone {drone_id}: {battery_level}')
        # TODO: Add drone_id
        # self._web_socket_server.send('battery-level', battery_level)

    def _log_orientation_callback(self, drone_id, data):
        measurements = {
            'roll': data['stateEstimate.roll'],
            'pitch': data['stateEstimate.pitch'],
            'yaw': data['stateEstimate.yaw'],
        }
        print(f'Orientation from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value:.2f}')

    def _log_position_callback(self, drone_id, data):
        measurements = {
            'x': data['stateEstimate.x'],
            'y': data['stateEstimate.y'],
            'z': data['stateEstimate.z'],
        }
        self._map_generator.add_position(measurements)
        print(f'Position from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value:.6f}')

    def _log_range_callback(self, drone_id, data):
        measurements = {
            'front': data['range.front'],
            'left': data['range.left'],
            'back': data['range.back'],
            'right': data['range.right'],
            'up': data['range.up'],
            'zrange': data['range.zrange'],
        }
        self._map_generator.add_points(measurements)
        print(f'Range from drone {drone_id}:')
        for key, value in measurements.items():
            print(f'- {key}: {value}')
