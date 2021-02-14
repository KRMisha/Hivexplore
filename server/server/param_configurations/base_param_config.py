class BaseParamConfig:
    def __init__(self, crazyflie):
        self._crazyflie = crazyflie

    @staticmethod
    def _update_callback(name, value):
        print(f'Readback: {name}={value}')
