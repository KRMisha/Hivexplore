from server.core.web_socket_server import WebSocketServer
from server.core.map_generator import MapGenerator
from server.core.unix_socket_client import UnixSocketClient


class ArgosManager:
    def __init__(self, web_socket_server: WebSocketServer, map_generator: MapGenerator):
        self._web_socket_server = web_socket_server
        self._map_generator = map_generator
        self._unix_socket_client = UnixSocketClient()

    async def start(self):
        await self._unix_socket_client.serve()

        # Client bindings
        self._web_socket_server.bind('set-led', self._set_led_enabled)

        # Argos bindings
        self._unix_socket_client.bind('pm.batteryLevel', self._get_battery_callback)
        # TODO: Handle ['stabilizer.roll', 'stabilizer.pitch', 'stabilizer.yaw']
        # TODO: Handle ['range.front', 'range.back', 'range.up', 'range.left', 'range.right', 'range.zrange']
        # TODO: Handle ['stateEstimate.x', 'stateEstimate.y', 'stateEstimate.z']

        # self._unix_socket_client.bind('pm.batteryLevel', self._mock_argos_battery_callback) # TODO: Remove
        # self._unix_socket_client.bind('range.front', self._mock_argos_range_front_callback) # TODO: Remove
        # self._unix_socket_client.bind('range.left', self._mock_argos_range_left_callback)# TODO: Remove
        # self._unix_socket_client.bind('range.back', self._mock_argos_range_back_callback)# TODO: Remove
        # self._unix_socket_client.bind('range.right', self._mock_argos_range_right_callback)# TODO: Remove
        # self._unix_socket_client.bind('stateEstimate.x', self._mock_argos_state_estimate_x_callback)# TODO: Remove
        # self._unix_socket_client.bind('stateEstimate.y', self._mock_argos_state_estimate_y_callback)# TODO: Remove
        # self._unix_socket_client.bind('stateEstimate.z', self._mock_argos_state_estimate_z_callback)# TODO: Remove

    def _set_led_enabled(self, is_enabled: bool):
        # TODO: Get drone IDs on connection to ARGoS and loop for all drone IDs
        drone_id = 's0'
        self._unix_socket_client.send('hivexplore.isM1LedOn', drone_id, is_enabled)

    def _get_battery_callback(self, drone_id, value):
        print(f'Received battery level from drone {drone_id}: {value}')
        # TODO: Add drone_id
        self._web_socket_server.send('battery-level', value)

    # TODO: Remove
    def _mock_argos_battery_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received battery level from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_range_front_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received range front from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_range_left_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received range left from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_range_back_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received range back from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_range_right_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received range right from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_state_estimate_x_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received state estimate x from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_state_estimate_y_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received state estimate y from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_state_estimate_z_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received state estimate z from drone {drone_id}: {value}')
