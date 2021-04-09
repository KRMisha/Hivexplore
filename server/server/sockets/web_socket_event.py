import enum


# Use non-standard enum names to match ...
# pylint: disable=invalid-name
class WebSocketEvent(enum.Enum):
    Connect = 'connect'
    MissionState = 'mission-state'
    DroneIds = 'drone-ids'
    Velocity = 'velocity'
    BatteryLevel = 'battery-level'
    DroneStatus = 'drone-status'
    SetLed = 'set-led'
    Log = 'log'
    MapPoints = 'map-points'
    ClearMap = 'clear-map'
    DronePosition = 'drone-position'
    DroneSensorLines = 'drone-sensor-lines'
