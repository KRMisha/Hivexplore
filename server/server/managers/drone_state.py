from enum import IntEnum


class DroneState(IntEnum):
    Standby = 0
    Flying = 1
    Crashed = 2
