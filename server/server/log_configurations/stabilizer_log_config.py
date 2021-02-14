from cflib.crazyflie.log import LogConfig
from server.log_configurations.base_log_config import BaseLogConfig


class StabilizerLogConfig(BaseLogConfig):
    def __init__(self):
        super().__init__()
        self.log_config = LogConfig(name='Stabilizer', period_in_ms=BaseLogConfig.PERIOD_MS)
        self.log_config.add_variable('stabilizer.roll', 'float')
        self.log_config.add_variable('stabilizer.pitch', 'float')
        self.log_config.add_variable('stabilizer.yaw', 'float')

    def log_data_cb(self, timestamp, data, logconf):
        print(f'{logconf.name}')
        print(f'- Roll: {data["stabilizer.roll"]:.2f}')
        print(f'- Pitch: {data["stabilizer.pitch"]:.2f}')
        print(f'- Yaw: {data["stabilizer.yaw"]:.2f}')
