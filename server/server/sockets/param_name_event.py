import enum


# Use non-standard enum names to match ...
# pylint: disable=invalid-name
class ParamNameEvent(enum.Enum):
    MissionState = 'missionState'
    IsM1LedOn = 'isM1LedOn'
