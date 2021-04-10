from enum import IntEnum


# Use non-standard enum names to match client string enum
# pylint: disable=invalid-name
class DroneStatus(IntEnum):
    Standby = 0
    Liftoff = 1
    Flying = 2
    Landing = 3
    Landed = 4
    Crashed = 5
    Returning = 6
