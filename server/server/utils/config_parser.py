import json
from typing import Dict, List
from cflib.crazyflie.mem import MemoryElement
from cflib.crazyflie.mem.i2c_element import I2CElement
from cflib.crazyflie import Crazyflie
from cflib.utils.power_switch import PowerSwitch
from server.tuples import Point

CRAZYFLIES_CONFIG_FILENAME = 'server/config/crazyflies_config.json'


def load_crazyflie_initial_offset_from_base() -> Dict[str, Point]:
    with open(CRAZYFLIES_CONFIG_FILENAME, 'r+') as file:
        try:
            crazyflies_config = json.load(file)
            return {crazyflie_config['uri']: Point(**crazyflie_config['initial-position']) for crazyflie_config in crazyflies_config}
        except ValueError:
            print('load_crazyflie_positions_from_file error: Could not load initial positions from file')
            raise


def load_crazyflie_uris_from_file() -> List[str]:
    with open(CRAZYFLIES_CONFIG_FILENAME, 'r+') as file:
        try:
            crazyflies_config = json.load(file)
            return [crazyflie_config['uri'] for crazyflie_config in crazyflies_config]
        except ValueError:
            print('load_crazyflie_uris_from_file error: Could not load URIs from file')
            raise


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
    old_address = crazyflie.link_uri.split('/')[-1]
    new_address = format(eeprom.elements['radio_address'], 'X')
    new_uri = crazyflie.link_uri.replace(old_address, new_address)

    print(f'Changed URI of drone {crazyflie.link_uri} to {new_uri}')
    print('Restarting drone...')

    PowerSwitch(crazyflie.link_uri).stm_power_cycle()

    with open(CRAZYFLIES_CONFIG_FILENAME, 'r') as file:
        crazyflies_config = json.load(file)

    for crazyflie_config in crazyflies_config:
        if crazyflie_config['uri'] == crazyflie.link_uri:
            crazyflie_config['uri'] = new_uri
            break
    else:
        crazyflies_config.append({'uri': new_uri, 'initial-position': {'x': 0, 'y': 0, 'z': 0}})

    with open(CRAZYFLIES_CONFIG_FILENAME, 'w') as file:
        json.dump(crazyflies_config, file)
