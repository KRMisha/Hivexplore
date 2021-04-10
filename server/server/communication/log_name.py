import enum


class LogName(enum.Enum):
    DRONE_IDS = 'drone-ids' # only used for ARGoS
    BATTERY_LEVEL = 'battery-level'
    ORIENTATION = 'orientation'
    POSITION = 'position'
    VELOCITY = 'velocity'
    RANGE = 'range'
    RSSI = 'rssi'
    DRONE_STATUS = 'drone-status'
    CONSOLE = 'console'
