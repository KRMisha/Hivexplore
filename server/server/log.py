from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig
from log_configurations.log_config_wrapper import LogConfigWrapper
from log_configurations.log_health import LogHealth
from log_configurations.log_stabilizer import LogStabilizer


class Log:
    def __init__(self, crazyflie: Crazyflie):
        self._crazyflie = crazyflie

        self._log_config_wrappers = [LogHealth(), LogStabilizer()]

    def start_logging(self):
        try:
            for config_wrapper in self._log_config_wrappers:
                log_config = config_wrapper._logConfig

                self._crazyflie.log.add_config(log_config)
                log_config.data_received_cb.add_callback(config_wrapper.log_data)
                log_config.error_cb.add_callback(config_wrapper.log_error)
                log_config.start()

        except KeyError as e:
            print('Could not start logging data,' '{} was not found in the Crazyflie table of content'.format(str(e)))
        except AttributeError as e:
            print('Could not add log configuration,' 'error: {}'.format(str(e)))
