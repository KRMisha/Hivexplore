import enum


class ParamName(enum.Enum):
    MISSION_STATE = 'missionState'
    IS_LED_ENABLED = 'isLedEnabled'
    BASE_OFFSET_X = 'baseOffsetX' # Only for Crazyflie
    BASE_OFFSET_Y = 'baseOffsetY' # Only for Crazyflie
    BASE_OFFSET_Z = 'baseOffsetZ' # Only for Crazyflie
