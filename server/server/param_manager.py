from server.param_configurations.led_param_config import LedParamConfig


class ParamManager:
    def __init__(self, crazyflie):
        # TODO Add more params in the future
        self.led_param = LedParamConfig(crazyflie)
