import enum


class WebSocketEvent(enum.Enum):
    Connect = 'connect'
    Disconnect = 'disconnect'
    DroneIds = 'drone-ids'
    BatteryLevel = 'battery-level'
    Orientation = 'orientation'
    Position = 'position'
    Velocity = 'velocity'
    Range = 'range'
    Rssi = 'rssi'
    DroneStatus = 'drone-status'
    Log = 'log'
    Console = 'console'
    MissionState = 'mission-state'
    SetLed = 'set-led'
    MapPoints = 'map-points'
    ClearMap = 'clear-map'
    DronePosition = 'drone-position'
    DroneSensorLines = 'drone-sensor-lines'
