from abc import ABC, abstractmethod
import logging
from typing import Any, Dict, List
import numpy as np
from server.communication.param_name import ParamName
from server.communication.web_socket_event import WebSocketEvent
from server.communication.web_socket_server import WebSocketServer
from server.logger import Logger
from server.managers.drone_status import DroneStatus
from server.managers.mission_state import MissionState
from server.map_generator import MapGenerator
from server.tuples import Orientation, Point, Range, Velocity


class DroneManager(ABC):
    def __init__(self, web_socket_server: WebSocketServer, logger: Logger, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._logger = logger
        self._map_generator = map_generator
        self._mission_state = MissionState.Standby
        self._drone_statuses: Dict[str, DroneStatus] = {}
        self._drone_leds: Dict[str, bool] = {}
        self._drone_battery_levels: Dict[str, int] = {}
        self._are_all_drones_charged = False

        # Client bindings
        self._web_socket_server.bind(WebSocketEvent.CONNECT, self._web_socket_connect_callback)
        self._web_socket_server.bind(WebSocketEvent.MISSION_STATE, self._set_mission_state)
        self._web_socket_server.bind(WebSocketEvent.LED, self._set_led_enabled)

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
        self._logger.log_drone_data(logging.INFO, drone_id, f'Set {param}: {value}')

    @abstractmethod
    def _get_drone_base_offset(self, drone_id: str) -> Point:
        pass

    def _send_drone_ids(self, client_id=None):
        if client_id is None:
            self._web_socket_server.send_message(WebSocketEvent.DRONE_IDS, self._get_drone_ids())
        else:
            self._web_socket_server.send_message_to_client(client_id, WebSocketEvent.DRONE_IDS, self._get_drone_ids())

    # Drone callbacks

    def _log_battery_callback(self, drone_id: str, data: Dict[str, int]):
        battery_level = data['pm.batteryLevel']
        self._drone_battery_levels[drone_id] = battery_level
        self._logger.log_drone_data(logging.INFO, drone_id, f'Battery level: {battery_level}')
        self._web_socket_server.send_drone_message(WebSocketEvent.BATTERY_LEVEL, drone_id, battery_level)

        try:
            MINIMUM_BATTERY_LEVEL = 30
            are_all_drones_charged = all(self._drone_battery_levels[id] >= MINIMUM_BATTERY_LEVEL for id in self._get_drone_ids())
        except KeyError:
            are_all_drones_charged = False

        if are_all_drones_charged != self._are_all_drones_charged:
            self._are_all_drones_charged = are_all_drones_charged
            self._web_socket_server.send_message(WebSocketEvent.ARE_ALL_DRONES_CHARGED, self._are_all_drones_charged)

    def _log_orientation_callback(self, drone_id, data: Dict[str, float]):
        orientation = Orientation(
            roll=data['stateEstimate.roll'],
            pitch=data['stateEstimate.pitch'],
            yaw=data['stateEstimate.yaw'],
        )

        self._logger.log_drone_data(logging.INFO, drone_id, f'Orientation: {orientation}')
        self._map_generator.set_orientation(drone_id, orientation)

    def _log_position_callback(self, drone_id: str, data: Dict[str, float]):
        base_offset = self._get_drone_base_offset(drone_id)
        point = Point(
            x=data['stateEstimate.x'] + base_offset.x,
            y=data['stateEstimate.y'] + base_offset.y,
            z=data['stateEstimate.z'] + base_offset.z,
        )

        self._logger.log_drone_data(logging.INFO, drone_id, f'Position: {point}')
        self._map_generator.set_position(drone_id, point)

    def _log_velocity_callback(self, drone_id: str, data: Dict[str, float]):
        velocity = Velocity(
            vx=data['stateEstimate.vx'],
            vy=data['stateEstimate.vy'],
            vz=data['stateEstimate.vz'],
        )

        velocity_magnitude = np.linalg.norm(list(velocity))
        self._logger.log_drone_data(logging.INFO, drone_id, f'Velocity: {velocity} | Magnitude: {velocity_magnitude}')
        self._web_socket_server.send_drone_message(WebSocketEvent.VELOCITY, drone_id, round(velocity_magnitude, 3))

    def _log_range_callback(self, drone_id: str, data: Dict[str, float]):
        range_reading = Range(
            front=data['range.front'],
            left=data['range.left'],
            back=data['range.back'],
            right=data['range.right'],
            up=data['range.up'],
            down=data['range.zrange'],
        )

        self._logger.log_drone_data(logging.INFO, drone_id, f'Range reading: {range_reading}')
        if self._mission_state != MissionState.Standby:
            self._map_generator.add_range_reading(drone_id, range_reading)

    def _log_rssi_callback(self, drone_id: str, data: Dict[str, float]):
        rssi = data['radio.rssi']
        self._logger.log_drone_data(logging.INFO, drone_id, f'RSSI: {rssi}')

    def _log_drone_status_callback(self, drone_id: str, data: Dict[str, int]):
        drone_status = DroneStatus(data['hivexplore.droneStatus'])
        self._logger.log_drone_data(logging.INFO, drone_id, f'Status: {drone_status.name}')

        self._drone_statuses[drone_id] = drone_status
        self._web_socket_server.send_drone_message(WebSocketEvent.DRONE_STATUS, drone_id, drone_status.name)

        try:
            are_all_drones_landed = all(self._drone_statuses[id] == DroneStatus.Landed for id in self._get_drone_ids())
        except KeyError:
            self._logger.log_server_data(logging.WARNING, 'DroneManager warning: At least one drone\'s status is unknown')
            are_all_drones_landed = False

        if are_all_drones_landed and (self._mission_state == MissionState.Returning or self._mission_state == MissionState.Emergency):
            self._set_mission_state(MissionState.Landed.name)

    def _log_console_callback(self, drone_id: str, data: str):
        self._logger.log_drone_data(logging.INFO, drone_id, f'Debug print: {data}')

    # Client callbacks

    def _web_socket_connect_callback(self, client_id: str):
        self._send_drone_ids(client_id)
        self._web_socket_server.send_message_to_client(client_id, WebSocketEvent.MISSION_STATE, self._mission_state.name)
        self._web_socket_server.send_message_to_client(client_id, WebSocketEvent.ARE_ALL_DRONES_CHARGED, self._are_all_drones_charged)

        for drone_id, is_led_enabled in self._drone_leds.items():
            self._web_socket_server.send_drone_message_to_client(client_id, WebSocketEvent.LED, drone_id, is_led_enabled)

    def _set_mission_state(self, mission_state_str: str):
        try:
            new_mission_state = MissionState[mission_state_str]
        except KeyError:
            self._logger.log_server_data(logging.ERROR, f'DroneManager error: Unknown mission state received: {mission_state_str}')
            return

        if new_mission_state == MissionState.Exploring:
            # Deny changing mission state to Exploring if a drone is under 30% battery
            if not self._are_all_drones_charged:
                self._logger.log_server_data(
                    logging.WARNING,
                    'DroneManager warning: Could not start mission since not all drones have a minimum battery level of 30%')
                self._web_socket_server.send_message(WebSocketEvent.MISSION_STATE, self._mission_state.name)
                return

            # Deny changing mission state to Exploring if no drones are connected
            if len(self._get_drone_ids()) == 0:
                self._logger.log_server_data(logging.WARNING, 'DroneManager warning: Could not start mission since no drones are connected')
                self._web_socket_server.send_message(WebSocketEvent.MISSION_STATE, self._mission_state.name)
                return

        self._mission_state = new_mission_state

        self._logger.log_server_data(logging.INFO, f'Set mission state: {self._mission_state}')
        for drone_id in self._get_drone_ids():
            self._set_drone_param(f'hivexplore.{ParamName.MISSION_STATE.value}', drone_id, self._mission_state)
        self._web_socket_server.send_message(WebSocketEvent.MISSION_STATE, mission_state_str)

        if self._mission_state == MissionState.Exploring:
            self._map_generator.clear()

        if self._mission_state == MissionState.Standby:
            self._logger.setup_logging()

    def _set_led_enabled(self, drone_id: str, is_enabled: bool):
        if self._is_drone_id_valid(drone_id):
            self._logger.log_server_data(logging.INFO, f'Set LED for {drone_id}: {is_enabled}')
            self._set_drone_param(f'hivexplore.{ParamName.IS_LED_ENABLED.value}', drone_id, is_enabled)
            self._web_socket_server.send_drone_message(WebSocketEvent.LED, drone_id, is_enabled)
            self._drone_leds[drone_id] = is_enabled
        else:
            self._logger.log_server_data(logging.ERROR, f'DroneManager error: Unknown drone ID received: {drone_id}')
