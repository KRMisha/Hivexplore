import logging
import random
import time

import cflib.crtp
from cflib.crazyflie import Crazyflie


class BaseParamConfig:
    def __init__(self, crazyflie):
        pass

    def update_callback(self, name, value):
        print(f'Readback: {name}={value}')
