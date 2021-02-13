from cflib.crazyflie.log import LogConfig


class LogConfigWrapper:

    PERIOD_MS = 1000

    def __init__(self):
        self._logConfig: LogConfig

    def log_data(self, timestamp, data, logconf):
        print(f'[{timestamp}][{logconf.name}]: {data}')

    def log_error(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')
