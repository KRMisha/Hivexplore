#!/usr/bin/env python3
"""
Main module.
"""
import time
import cflib
from connection_crazyflie import ConnectionCrazyflie

if __name__ == '__main__':

    cflib.crtp.init_drivers(enable_debug_driver=False)

    print('Scanning interfaces for Crazyflies...')
    available = cflib.crtp.scan_interfaces()

    print('Crazyflies found:')
    for i in available:
        print(i[0])

    crazyflies:list = []
    if len(available) > 0:
        for crazyflie in available:
            crazyflies.append(ConnectionCrazyflie(crazyflie[0]))
    else:
        print('No Crazyflies found, cannot run example')

    # The Crazyflie lib doesn't contain anything to keep the application alive,
    # so this is where your application should do something. In our case we
    # are just waiting until we are disconnected.
    while crazyflies[0].is_connected:
        time.sleep(1)
