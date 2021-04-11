import enum


class WebSocketEvent(str, enum.Enum):
    CONNECT = 'connect'
    MISSION_STATE = 'mission-state'
    DRONE_IDS = 'drone-ids'
    ARE_ALL_DRONES_CHARGED = 'are-all-drones-charged'
    MAP_POINTS = 'map-points'
    CLEAR_MAP = 'clear-map'
    DRONE_POSITION = 'drone-position'
    DRONE_SENSOR_LINES = 'drone-sensor-lines'
    BATTERY_LEVEL = 'battery-level'
    VELOCITY = 'velocity'
    DRONE_STATUS = 'drone-status'
    LED = 'led'
    LOG = 'log'
