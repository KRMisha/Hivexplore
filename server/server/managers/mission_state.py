from enum import IntEnum


# Use non-standard enum names to match client string enum
# pylint: disable=invalid-name
class MissionState(IntEnum):
    Standby = 0
    Exploring = 1
    Returning = 2
    Landing = 3
