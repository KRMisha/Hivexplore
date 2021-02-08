from cflib.crazyflie.log import LogConfig

class LogConfigWrapperInterface:

    def __init__(self):
        self._logConfig: LogConfig
        pass

    def log_data(self):
        pass
    
    def log_error(self):
        pass