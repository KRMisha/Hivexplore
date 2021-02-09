from cflib.crazyflie.log import LogConfig
from .log_config_wrapper import LogConfigWrapper


class LogHealth(LogConfigWrapper):
    def __init__(self):
        self._logConfig = LogConfig(name='DroneHealth', period_in_ms=LogConfigWrapper.PERIOD_MS)
        self._logConfig.add_variable('pm.vbat')

    def log_data(self, timestamp, data, logconf):
        battery_level = self.convert_voltage_to_percentage(data['pm.vbat'])
        print('Battery Level: %.2f' % (battery_level))

    def convert_voltage_to_percentage(self, voltage):
        MIN_VOLTAGE = 3.0
        MAX_VOLTAGE = 4.23
        MAX_BATTERY_LEVEL = 100
        battery_level = (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * MAX_BATTERY_LEVEL

        return max(0, min(battery_level, MAX_BATTERY_LEVEL))
