from typing import Any, Dict, List

class MapGenerator:
    def __init__(self):
        self._data: List[Dict[str, Any]] = []

    def add_points(self, data: Dict[str, Any]):
        self._data.append(data)
