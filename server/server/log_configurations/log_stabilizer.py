from cflib.crazyflie.log import LogConfig
from .log_config_wrapper import LogConfigWrapper


class LogStabilizer(LogConfigWrapper):
    def __init__(self):
        self._logConfig = LogConfig(name='Stabilizer', period_in_ms=LogConfigWrapper.PERIOD_MS)
        self._logConfig.add_variable('stabilizer.roll', 'float')
        self._logConfig.add_variable('stabilizer.pitch', 'float')
        self._logConfig.add_variable('stabilizer.yaw', 'float')

    def log_data(self, timestamp, data, logconf):
        print('%s' % logconf.name)
        print('- Roll: %.2f' % data['stabilizer.roll'])
        print('- Pitch: %.2f' % data['stabilizer.pitch'])
        print('- Yaw: %.2f' % data['stabilizer.yaw'])
