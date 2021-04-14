from enum import IntEnum


# Use non-standard enum names to match client string enum
# pylint: disable=invalid-name
class DroneStatus(IntEnum):
    Standby = 0
    Liftoff = 1
    Flying = 2
    Returning = 3
    Landing = 4
    Landed = 5
    Crashed = 6
