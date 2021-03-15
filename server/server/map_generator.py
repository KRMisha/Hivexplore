import math
from collections import namedtuple
from typing import Dict, List
import numpy as np
from server.sockets.web_socket_server import WebSocketServer

Orientation = namedtuple('Orientation', ['roll', 'pitch', 'yaw'])
Point = namedtuple('Point', ['x', 'y', 'z'])
Range = namedtuple('Range', ['front', 'left', 'back', 'right', 'up', 'down'])


class MapGenerator:
    def __init__(self, web_socket_server: WebSocketServer):
        self._web_socket_server = web_socket_server
        self._last_orientations: Dict[str, Orientation] = {}
        self._last_positions: Dict[str, Point] = {}
        self._points: List[Point] = []

    def set_orientation(self, drone_id: str, orientation: Orientation):
        self._last_orientations[drone_id] = orientation

    def set_position(self, drone_id: str, position: Point):
        self._last_positions[drone_id] = position

    def add_range_reading(self, drone_id: str, range_reading: Range):
        points = self._calculate_points_from_readings(self._last_orientations[drone_id], self._last_positions[drone_id], range_reading)
        print(f'Points detected by drone {drone_id}: {points}')
        self._web_socket_server.send_message('map-points', points)

    def _calculate_points_from_readings(self, last_orientation: Orientation, last_position: Point, range_reading: Range) -> List[Point]:
        IS_DOWN_SENSOR_PLOTTING_ENABLED = False
        SENSOR_THRESHOLD = 2000
        METER_TO_MILLIMETER_FACTOR = 1000

        detected_points = []

        if range_reading.front < SENSOR_THRESHOLD:
            point = Point(
                last_position.x + range_reading.front / METER_TO_MILLIMETER_FACTOR,
                last_position.y,
                last_position.z,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        if range_reading.left < SENSOR_THRESHOLD:
            point = Point(
                last_position.x,
                last_position.y + range_reading.left / METER_TO_MILLIMETER_FACTOR,
                last_position.z,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        if range_reading.back < SENSOR_THRESHOLD:
            point = Point(
                last_position.x - range_reading.back / METER_TO_MILLIMETER_FACTOR,
                last_position.y,
                last_position.z,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        if range_reading.right < SENSOR_THRESHOLD:
            point = Point(
                last_position.x,
                last_position.y - range_reading.right / METER_TO_MILLIMETER_FACTOR,
                last_position.z,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        if range_reading.up < SENSOR_THRESHOLD:
            point = Point(
                last_position.x,
                last_position.y,
                last_position.z + range_reading.up / METER_TO_MILLIMETER_FACTOR,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        if range_reading.down < SENSOR_THRESHOLD and IS_DOWN_SENSOR_PLOTTING_ENABLED:
            point = Point(
                last_position.x,
                last_position.y,
                last_position.z - range_reading.down / METER_TO_MILLIMETER_FACTOR,
            )
            detected_points.append(self._rotate_point(last_orientation, last_position, point))

        return detected_points

    @staticmethod
    def _rotate_point(orientation: Orientation, origin: Point, point: Point) -> Point:
        cos_roll = math.cos(math.radians(orientation.roll))
        cos_pitch = math.cos(math.radians(orientation.pitch))
        cos_yaw = math.cos(math.radians(orientation.yaw))

        sin_roll = math.sin(math.radians(orientation.roll))
        sin_pitch = math.sin(math.radians(orientation.pitch))
        sin_yaw = math.sin(math.radians(orientation.yaw))

        yaw_rotation = np.array([
            [cos_yaw, -sin_yaw, 0],
            [sin_yaw, cos_yaw, 0],
            [0, 0, 1],
        ])
        pitch_rotation = np.array([
            [cos_pitch, 0, sin_pitch],
            [0, 1, 0],
            [-sin_pitch, 0, cos_pitch],
        ])
        roll_rotation = np.array([
            [1, 0, 0],
            [0, cos_roll, -sin_roll],
            [0, sin_roll, cos_roll],
        ])

        rotation_matrix = np.array(yaw_rotation @ pitch_rotation @ roll_rotation)

        rotated_point = (rotation_matrix @ (np.subtract(point, origin))) + origin

        return Point(*rotated_point)