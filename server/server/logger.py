from cflib.crazyflie import Crazyflie
from server.log_configurations.battery_log_config import BatteryLogConfig
from server.log_configurations.stabilizer_log_config import StabilizerLogConfig


class Logger:
    def __init__(self, crazyflie: Crazyflie):
        self._crazyflie = crazyflie
        self._log_configs = [BatteryLogConfig(), StabilizerLogConfig()]

    def start_logging(self):
        try:
            for log_config in self._log_configs:
                self._crazyflie.log.add_config(log_config.log_config)
                log_config.start()
        except KeyError as key_error:
            print(f'Could not start logging data, {key_error} was not found in the Crazyflie TOC')
        except AttributeError as attribute_error:
            print(f'Could not add log configuration, error: {attribute_error}')
