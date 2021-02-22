from typing import Any, Dict, List


class MapGenerator:
    def __init__(self):
        self._points: List[Dict[str, Any]] = []
        self._position: List[Dict[str, Any]] = []

    def add_points(self, data: Dict[str, Any]):
        self._points.append(data)

    def add_position(self, data: Dict[str, Any]):
        self._position.append(data)
