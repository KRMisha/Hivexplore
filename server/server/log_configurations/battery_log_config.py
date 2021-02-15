from cflib.crazyflie.log import LogConfig
from server.log_configurations.base_log_config import BaseLogConfig


class BatteryLogConfig(BaseLogConfig):
    def __init__(self):
        super().__init__()
        self.log_config = LogConfig(name='BatteryLevel', period_in_ms=BaseLogConfig.PERIOD_MS)
        self.log_config.add_variable('pm.vbat')
        self.battery_level: int = 0

    def log_data_cb(self, timestamp, data, logconf):
        self.battery_level = self._get_battery_percentage(data['pm.vbat'])
        print(f'Battery level: {self.battery_level:.2f}')

    @staticmethod
    def _get_battery_percentage(voltage) -> int:
        MIN_VOLTAGE = 3.0
        MAX_VOLTAGE = 4.23
        MAX_BATTERY_LEVEL = 100
        battery = (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * MAX_BATTERY_LEVEL

        return int(max(0, min(battery, MAX_BATTERY_LEVEL)))
