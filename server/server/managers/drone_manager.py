from abc import ABC, abstractmethod
from collections import namedtuple
from typing import Any, Dict, List
import numpy as np
from server.managers.drone_status import DroneStatus
from server.managers.mission_state import MissionState
from server.map_generator import MapGenerator, Orientation, Point, Range
from server.sockets.web_socket_server import WebSocketServer


class DroneManager(ABC):
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
        self._drone_statuses: Dict[str, DroneStatus] = {}

        # Client bindings
        self._web_socket_server.bind('connect', self._web_socket_connect_callback)

    @abstractmethod
    async def start(self):
        pass

    @abstractmethod
    def _get_drone_ids(self) -> List[str]:
        pass

    @abstractmethod
    def _is_drone_id_valid(self, drone_id: str) -> bool:
        pass

    @abstractmethod
    def _set_drone_param(self, param: str, drone_id: str, value: Any):
        pass

    def _send_drone_ids(self, client_id=None):
        if client_id is None:
            self._web_socket_server.send_message('drone-ids', self._get_drone_ids())
        else:
            self._web_socket_server.send_message_to_client(client_id, 'drone-ids', self._get_drone_ids())

    # Drone callbacks

    def _log_battery_callback(self, drone_id: str, data: Dict[str, int]):
        battery_level = data['pm.batteryLevel']
        print(f'BatteryLevel from drone {drone_id}: {battery_level}')
        self._web_socket_server.send_drone_message('battery-level', drone_id, battery_level)

    def _log_orientation_callback(self, drone_id, data: Dict[str, float]):
        orientation = Orientation(
            roll=data['stateEstimate.roll'],
            pitch=data['stateEstimate.pitch'],
            yaw=data['stateEstimate.yaw'],
        )
        print(f'Orientation from drone {drone_id}: {orientation}')
        self._map_generator.set_orientation(drone_id, orientation)

    def _log_position_callback(self, drone_id: str, data: Dict[str, float]):
        point = Point(
            x=data['stateEstimate.x'],
            y=data['stateEstimate.y'],
            z=data['stateEstimate.z'],
        )
        print(f'Position from drone {drone_id}: {point}')
        self._map_generator.set_position(drone_id, point)

    def _log_velocity_callback(self, drone_id: str, data: Dict[str, float]):
        Velocity = namedtuple('Velocity', ['vx', 'vy', 'vz'])
        velocity = Velocity(
            vx=data['stateEstimate.vx'],
            vy=data['stateEstimate.vy'],
            vz=data['stateEstimate.vz'],
        )
        velocity_magnitude = np.linalg.norm(list(velocity))
        print(f'Velocity from drone {drone_id}: {velocity} | Magnitude: {velocity_magnitude}')
        self._web_socket_server.send_drone_message('velocity', drone_id, round(velocity_magnitude, 4))

    def _log_range_callback(self, drone_id: str, data: Dict[str, float]):
        range_reading = Range(
            front=data['range.front'],
            left=data['range.left'],
            back=data['range.back'],
            right=data['range.right'],
            up=data['range.up'],
            down=data['range.zrange'],
        )
        print(f'Range from drone {drone_id}: {range_reading}')
        self._map_generator.add_range_reading(drone_id, range_reading)

    @staticmethod
    def _log_rssi_callback(drone_id: str, data: Dict[str, float]):
        rssi = data['radio.rssi']
        print(f'RSSI from drone {drone_id}: {rssi}')

    def _log_drone_status_callback(self, drone_id: str, data: Dict[str, int]):
        drone_status = data['hivexplore.droneStatus']
        print(f'Drone status from drone {drone_id}: {drone_status}')

        drone_status_value = DroneStatus(drone_status)
        self._drone_statuses[drone_id] = drone_status_value
        self._web_socket_server.send_drone_message('drone-status', drone_id, drone_status_value.name)

        are_all_drones_landed = all(self._drone_statuses[id] == DroneStatus.Landed for id in self._get_drone_ids())
        if are_all_drones_landed:
            print(f'Set mission state: {MissionState.Landed.name}')
            self._web_socket_server.send_message('mission-state', MissionState.Landed.name)
            self._set_drone_param('hivexplore.missionState', drone_id, MissionState.Landed.name)


    def _log_console_callback(self, drone_id: str, data: str):
        print(f'Debug print from drone {drone_id}: {data}')
        # TODO: send console log to client through self._web_socket_server.send_drone_message

    # Client callbacks

    def _web_socket_connect_callback(self, client_id: str):
        self._send_drone_ids(client_id)

    def _set_mission_state(self, mission_state_str: str):
        try:
            mission_state = MissionState[mission_state_str]
        except KeyError:
            print('ArgosManager error: Unknown mission state received:', mission_state_str)
            return

        print('Set mission state:', mission_state)
        for drone_id in self._get_drone_ids():
            self._set_drone_param('hivexplore.missionState', drone_id, mission_state)
        self._web_socket_server.send_message('mission-state', mission_state_str)

    def _set_led_enabled(self, drone_id: str, is_enabled: bool):
        if self._is_drone_id_valid(drone_id):
            print(f'Set LED state for drone {drone_id}: {is_enabled}')
            self._set_drone_param('hivexplore.isM1LedOn', drone_id, is_enabled)
            self._web_socket_server.send_drone_message('set-led', drone_id, is_enabled)
        else:
            print('CrazyflieManager error: Unknown drone ID received:', drone_id)
