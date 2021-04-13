import argparse
from server.server import Server


def main():
    try:
        parser = argparse.ArgumentParser(description='Hivexplore server.')

        subparsers = parser.add_subparsers(dest='mode', required=True)
        drone_parser = subparsers.add_parser('drone', help='use the Crazyradio to connect to Crazyflies')
        drone_parser.add_argument('--debug', action='store_true', help='enable the Crazyflie debug driver')
        subparsers.add_parser('argos', help='use ARGoS to simulate the drones')

        args = parser.parse_args()

        server = Server(args)
        server.start()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
