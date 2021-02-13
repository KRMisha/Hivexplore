from cflib.crazyflie.log import LogConfig

class BaseLogConfig:

    PERIOD_MS = 1000

    def __init__(self):
        self.log_config: LogConfig

    def log_data(self, timestamp, data, logconf):
        print(f'[{timestamp}][{logconf.name}]: {data}')

    def log_error(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')
