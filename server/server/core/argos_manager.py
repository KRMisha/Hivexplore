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
        self._unix_socket_client.bind('BatteryLevel', self._mock_argos_battery_callback) # TODO: Remove
        self._unix_socket_client.bind('Orientation', self._mock_argos_orientation_callback)# TODO: Remove
        self._unix_socket_client.bind('Position', self._mock_argos_position_callback)# TODO: Remove
        self._unix_socket_client.bind('Range', self._mock_argos_range_callback) # TODO: Remove

        await self._unix_socket_client.serve()

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
        print(f'Received battery level log from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_orientation_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received orientation log from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_position_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received position log from drone {drone_id}: {value}')

    # TODO: Remove
    def _mock_argos_range_callback(self, drone_id, value): # pylint: disable=no-self-use
        print(f'Received range log from drone {drone_id}: {value}')
