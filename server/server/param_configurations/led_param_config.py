from server.param_configurations.base_param_config import BaseParamConfig


class LedParamConfig(BaseParamConfig):
    def __init__(self, crazyflie):
        self._crazyflie = crazyflie
        super().__init__(crazyflie)
        self._group = 'hivexplore'
        self._name = 'isM1LedOn'

        self._crazyflie.param.add_update_callback(group=self._group, name=self._name, cb=self.update_callback)

    def update_callback(self, name, value):
        print(f'Readback: {name}={value}')

    def turn_on_led(self):
        led = 1
        self._crazyflie.param.set_value(f'{self._group}.{self._name}', f'{led}')

    def turn_off_led(self):
        led = 0
        self._crazyflie.param.set_value(f'{self._group}.{self._name}', f'{led}')
