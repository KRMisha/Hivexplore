import enum


# Use non-standard enum names to match WebSocketEvent enum
# pylint: disable=invalid-name
class UnixSocketEvent(enum.Enum):
    Disconnect = 'disconnect'
