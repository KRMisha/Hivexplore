from enum import IntEnum


class MissionState(IntEnum):
    STANDBY = 0
    MISSION = 1
    RETURNING = 2
