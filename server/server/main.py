#!/usr/bin/env python3

import time
import cflib
from server.crazyflie_connection import CrazyflieConnection


def main():
    cflib.crtp.init_drivers(enable_debug_driver=False)

    print('Scanning interfaces for Crazyflies...')
    available_interfaces = cflib.crtp.scan_interfaces()

    print('Crazyflies found:')
    for available_interface in available_interfaces:
        print(available_interface[0])

    crazyflies = []
    if len(available_interfaces) > 0:
        for crazyflie in available_interfaces:
            crazyflies.append(CrazyflieConnection(crazyflie[0]))
    else:
        print('No Crazyflies found, cannot run example')

    # The Crazyflie lib doesn't contain anything to keep the application alive,
    # so this is where your application should do something. In our case we
    # are just waiting until we are disconnected.
    while crazyflies[0].is_connected:
        time.sleep(1)


if __name__ == '__main__':
    main()
