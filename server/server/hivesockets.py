import asyncio
import websockets

IP_ADDRESS = "localhost"
PORT = 5678


# The following was inspired by: https://websockets.readthedocs.io/en/stable/intro.html#common-patterns

# TODO: Get this information from the drones
async def get_battery_handler(websocket, path):
    battery = 0
    while True:
        battery %= 100
        battery += 1
        await websocket.send(str(battery))
        await asyncio.sleep(1)


# TODO: Forward this command to the drone
async def toggle_led_handler(websocket, path):
    async for message in websocket:
        print(message)


async def socket_handler(websocket, path):
    toggle_led_task = asyncio.ensure_future(toggle_led_handler(websocket, path))
    get_battery_task = asyncio.ensure_future(get_battery_handler(websocket, path))

    done, pending = await asyncio.wait([toggle_led_task, get_battery_task], return_when=asyncio.FIRST_COMPLETED)
    for task in pending:
        task.cancel()


def start_hive_server():
    start_server = websockets.serve(socket_handler, IP_ADDRESS, PORT)
    asyncio.get_event_loop().run_until_complete(start_server)
    asyncio.get_event_loop().run_forever()


def main():
    start_hive_server()

# TODO: Remove this to integrate it with the drones
if __name__ == '__main__':
    main()
