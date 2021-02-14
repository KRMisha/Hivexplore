from cflib.crazyflie.log import LogConfig


class BaseLogConfig:
    PERIOD_MS = 1000

    def __init__(self):
        self.log_config: LogConfig

    def start(self):
        self.log_config.data_received_cb.add_callback(self.log_data_cb)
        self.log_config.error_cb.add_callback(self.log_error_cb)
        self.log_config.start()

    # pylint: disable=R0201
    def log_data_cb(self, timestamp, data, logconf):
        print(f'[{timestamp}][{logconf.name}]: {data}')

    def log_error_cb(self, logconf, msg):
        print(f'Error when logging {logconf.name}: {msg}')
