import sys
import cflib
from cflib.crazyflie import Crazyflie
from server.utils.config_parser import CRAZYFLIES_CONFIG_FILENAME, load_crazyflies_config, set_crazyflie_radio_address


def main():
    if len(sys.argv) != 3:
        print('Incorrect program usage.\nExample usage: python3 -m server.assign_crazyflie_address radio://0/80/2M/E7E7E7E7E7 E7E7E7E701')
        sys.exit(1)

    try:
        if sys.argv[1] not in load_crazyflies_config():
            print('Assigned URI not in \'{CRAZYFLIES_CONFIG_FILENAME}\', adding automatically (with a default base offset of (0, 0, 0))')
    except (FileNotFoundError, ValueError) as exc:
        print(f'assign_crazyflie_address error: Could not load Crazyflies config from \'{CRAZYFLIES_CONFIG_FILENAME}\': {exc}')
        sys.exit(1)

    cflib.crtp.init_drivers(enable_debug_driver=False)

    print('Trying to connect to:', sys.argv[1])
    crazyflie = Crazyflie(rw_cache='./cache')

    crazyflie.connected.add_callback(lambda link_uri: _connected(link_uri, crazyflie, sys.argv[2]))
    crazyflie.disconnected.add_callback(_disconnected)
    crazyflie.connection_failed.add_callback(_connection_failed)
    crazyflie.connection_lost.add_callback(_connection_lost)

    crazyflie.open_link(sys.argv[1])


def _connected(link_uri: str, crazyflie: Crazyflie, new_address: str):
    print(f'Connected to {link_uri}')
    set_crazyflie_radio_address(crazyflie, int(new_address, 16))


def _disconnected(link_uri: str):
    print(f'Disconnected from {link_uri}')


def _connection_failed(link_uri: str, msg: str):
    print(f'Connection to {link_uri} failed: {msg}')


def _connection_lost(link_uri: str, msg: str):
    print(f'Connection to {link_uri} lost: {msg}')


if __name__ == '__main__':
    main()
