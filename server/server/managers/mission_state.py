from enum import IntEnum


class MissionState(IntEnum):
    STANDBY = 0
    EXPLORING = 1
    RETURNING = 2