from collections import namedtuple

Orientation = namedtuple('Orientation', ['roll', 'pitch', 'yaw'])
Point = namedtuple('Point', ['x', 'y', 'z'])
Range = namedtuple('Range', ['front', 'left', 'back', 'right', 'up', 'down'])
Velocity = namedtuple('Velocity', ['vx', 'vy', 'vz'])
