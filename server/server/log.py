from cflib.crazyflie import Crazyflie
from cflib.crazyflie.log import LogConfig

PERIOD = 1000

class Log:

    def __init__(self, crazyflie: Crazyflie):
        self._crazyflie = crazyflie
        self._lg = LogConfig(name = 'DroneData', period_in_ms = PERIOD)
        self._lg.add_variable('pm.vbat')

    def start_logging(self):
        try:
            self._crazyflie.log.add_config(self._lg)

            self._lg.data_received_cb.add_callback(self.log_data)
            self._lg.error_cb.add_callback(self.log_error)
            self._lg.start()
        except KeyError as e:
            print('Could not start logging data,'
                  '{} was not found in TOC'.format(str(e)))
        except AttributeError:
            print('Could not add log configuration')

    def log_data(self, timestamp, data, logconf):
        battery_level = self.format_battery_level(data['pm.vbat'])
        print('Battery Level: %.2f' % (battery_level))

        # To print all log variables from log configuration
        # print('[%d][%s]: %s' % (timestamp, logconf.name, data))

    def log_error(self, timestamp, msg, logconf):
        print('[%d]: Logging error of %d: %s'% (timestamp, logconf.name, msg))

    def format_battery_level(self, voltage):
        # Temporary diplay of battery TO DO: change firmware to send formatted battery?
        battery_level = (voltage - 3.0)/ (4.23 - 3.0) * 100.0
        if battery_level < 0.0:
            battery_level = 0.0
        elif battery_level > 100.0:
            battery_level = 100.0
        return battery_level
