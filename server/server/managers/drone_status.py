from enum import IntEnum


# pylint: disable=invalid-name
class DroneStatus(IntEnum):
    Standby = 0
    Flying = 1
    Crashed = 2
