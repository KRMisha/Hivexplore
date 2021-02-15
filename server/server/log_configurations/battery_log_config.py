from cflib.crazyflie.log import LogConfig
from server.log_configurations.base_log_config import BaseLogConfig


class BatteryLogConfig(BaseLogConfig):
    def __init__(self):
        super().__init__()
        self.log_config = LogConfig(name='BatteryLevel', period_in_ms=BaseLogConfig.PERIOD_MS)
        self.log_config.add_variable('pm.vbat')

    def log_data_cb(self, timestamp, data, logconf):
        battery_level = self._get_battery_percentage(data['pm.vbat'])
        print(f'Battery level: {battery_level:.2f}')

    @staticmethod
    def _get_battery_percentage(voltage):
        MIN_VOLTAGE = 3.0
        MAX_VOLTAGE = 4.23
        MAX_BATTERY_LEVEL = 100
        battery_level = (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * MAX_BATTERY_LEVEL

        return max(0, min(battery_level, MAX_BATTERY_LEVEL))
