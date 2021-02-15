import asyncio
from distutils.util import strtobool
import websockets
from server.crazyflie_connection import CrazyflieConnection # TODO: How to import type without importing everything?

IP_ADDRESS = 'localhost'
PORT = 5678
BATTERY_SLEEP_TIME_S = 1


class SocketServer:
    def __init__(self, crazyflies):
        self._crazyflies = crazyflies

    async def _get_battery_handler(self, websocket, path):
        while True:
            battery = self._crazyflies[0].get_battery_percentage()
            battery_packet = str(battery) if battery is not None else str(0)
            await websocket.send(battery_packet)
            await asyncio.sleep(BATTERY_SLEEP_TIME_S)

    async def _toggle_led_handler(self, websocket, path):
        async for message in websocket:
            self._crazyflies[0].set_m1_led(strtobool(message))
            print(message)

    async def _socket_handler(self, websocket, path):
        toggle_led_task = asyncio.ensure_future(self._toggle_led_handler(websocket, path))
        get_battery_task = asyncio.ensure_future(self._get_battery_handler(websocket, path))

        done, pending = await asyncio.wait([toggle_led_task, get_battery_task], return_when=asyncio.FIRST_COMPLETED)
        for task in pending:
            task.cancel()

    def serve(self):
        server_instance = websockets.serve(self._socket_handler, IP_ADDRESS, PORT)
        asyncio.get_event_loop().run_until_complete(server_instance)
        asyncio.get_event_loop().run_forever()
