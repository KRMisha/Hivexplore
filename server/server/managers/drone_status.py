from enum import IntEnum


# Use non-standard enum names to match client string enum
# pylint: disable=invalid-name
class DroneStatus(IntEnum):
    Standby = 0
    Flying = 1
    Crashed = 2
