from cflib.crazyflie.log import LogConfig

class LogConfigWrapper:

    def __init__(self):
        self._logConfig: LogConfig

    def log_data(self, timestamp, data, logconf):
        print('[%d][%s]: %s' % (timestamp, logconf.name, data))
    
    def log_error(self, timestamp, data, logconf):
        print('[%d]: Logging error of %d: %s'% (timestamp, logconf.name, msg))
