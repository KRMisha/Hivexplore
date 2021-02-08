from cflib.crazyflie.log import LogConfig
from .log_config_wrapper import LogConfigWrapper 

PERIOD = 1000

class LogHealth(LogConfigWrapper):

    def __init__(self):
        self._logConfig = LogConfig(name = 'DroneHealth', period_in_ms = PERIOD)
        self._logConfig.add_variable('pm.vbat')

    def log_data(self, timestamp, data, logconf):
        battery_level = self.format_battery_level(data['pm.vbat'])
        print('Battery Level: %.2f' % (battery_level))

    def format_battery_level(self, voltage):
        battery_level = (voltage - 3.0)/ (4.23 - 3.0) * 100.0
        if battery_level < 0.0:
            battery_level = 0.0
        elif battery_level > 100.0:
            battery_level = 100.0
        return battery_level