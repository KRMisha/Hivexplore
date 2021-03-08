import numpy as np
from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator, Orientation, Point

# pylint: disable=no-self-use


class DroneManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator

    # Getters

    def _get_drone_ids(self):
        raise NotImplementedError()

    # Methods

    def _send_drone_ids(self, client_id=None):
        if client_id is None:
            self._web_socket_server.send_message('drone-ids', self._get_drone_ids())
        else:
            self._web_socket_server.send_message_to_client(client_id, 'drone-ids', self._get_drone_ids())

    # Drone callbacks

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

        velocity_magnitude = np.linalg.norm(list(measurements.values()))
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
        self._send_drone_ids(client_id)
