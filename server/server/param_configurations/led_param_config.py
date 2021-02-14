from server.param_configurations.base_param_config import BaseParamConfig


class LedParamConfig(BaseParamConfig):
    def __init__(self, crazyflie):
        super().__init__(crazyflie)
        self._group = 'hivexplore'
        self._name = 'isM1LedOn'

        self._crazyflie.param.add_update_callback(group=self._group, name=self._name, cb=self._update_callback)

    @staticmethod
    def _update_callback(name, value):
        print(f'Readback: {name}={value}')

    def set_led_enabled(self, is_enabled: bool):
        self._crazyflie.param.set_value(f'{self._group}.{self._name}', str(int(is_enabled)))
