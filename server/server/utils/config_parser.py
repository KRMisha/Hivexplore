import json
from typing import Any, Dict
from cflib.crazyflie.mem import MemoryElement
from cflib.crazyflie.mem.i2c_element import I2CElement
from cflib.crazyflie import Crazyflie
from cflib.utils.power_switch import PowerSwitch

CRAZYFLIES_CONFIG_FILENAME = 'server/config/crazyflies_config.json'


def load_crazyflies_config() -> Dict[str, Dict[str, Any]]:
    with open(CRAZYFLIES_CONFIG_FILENAME, 'r') as file:
        return json.load(file)


def set_crazyflie_radio_address(crazyflie: Crazyflie, radio_address: int):
    eeprom = crazyflie.mem.get_mems(MemoryElement.TYPE_I2C)[0]

    # Write default values in eeprom
    eeprom.elements['version'] = 1
    eeprom.elements['pitch_trim'] = 0.0
    eeprom.elements['roll_trim'] = 0.0
    eeprom.elements['radio_channel'] = 80
    eeprom.elements['radio_speed'] = 2 # 2 is the index pointing to 2M

    # Write radio address
    eeprom.elements['radio_address'] = radio_address

    eeprom.write_data(lambda eeprom, addr: _data_written(crazyflie, eeprom, addr))


def _data_written(crazyflie: Crazyflie, eeprom: I2CElement, _addr: int):
    eeprom.update(lambda eeprom: _data_updated(crazyflie, eeprom))


def _data_updated(crazyflie: Crazyflie, eeprom: I2CElement):
    old_uri = crazyflie.link_uri
    old_address = old_uri.split('/')[-1]
    new_address = format(eeprom.elements['radio_address'], 'X')
    new_uri = old_uri.replace(old_address, new_address)

    print(f'Changed URI of drone {old_uri} to {new_uri}')
    print('Restarting drone...')

    PowerSwitch(old_uri).stm_power_cycle()

    with open(CRAZYFLIES_CONFIG_FILENAME, 'r') as file:
        crazyflies_config = json.load(file)

    try:
        crazyflies_config[new_uri] = crazyflies_config[old_uri]
    except KeyError:
        crazyflies_config[new_uri] = {
            'baseOffset': {
                'x': 0,
                'y': 0,
                'z': 0,
            },
        }

    with open(CRAZYFLIES_CONFIG_FILENAME, 'w') as file:
        json.dump(crazyflies_config, file)
