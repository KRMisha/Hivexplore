import enum


class WebSocketEvent(str, enum.Enum):
    CONNECT = 'connect'
    MISSION_STATE = 'mission-state'
    DRONE_IDS = 'drone-ids'
    MAP_POINTS = 'map-points'
    CLEAR_MAP = 'clear-map'
    DRONE_POSITIONS = 'drone-position'
    DRONE_SENSOR_LINES = 'drone-sensor-lines'
    VELOCITY = 'velocity'
    BATTERY_LEVEL = 'battery-level'
    DRONE_STATUS = 'drone-status'
    LED = 'led'
    LOG = 'log'
