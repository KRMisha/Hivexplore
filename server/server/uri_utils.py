import json
from typing import List
from cflib.crazyflie.mem import MemoryElement
from cflib.crazyflie.mem.i2c_element import I2CElement
from cflib.crazyflie import Crazyflie
from cflib.utils.power_switch import PowerSwitch

CRAZYFLIE_URIS_FILENAME = 'server/config/crazyflie_uris.json'


def load_crazyflie_uris_from_file() -> List[str]:
    with open(CRAZYFLIE_URIS_FILENAME, 'r+') as uris_file:
        try:
            data = json.load(uris_file)
            uris = data['crazyflie_uris']
            return uris
        except ValueError:
            print('load_crazyflie_uris_from_file error: Can\'t load URIs from file')
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

    try:
        uris = load_crazyflie_uris_from_file()
    except ValueError:
        uris = []

    # Remove old drone URI
    try:
        uris.remove(crazyflie.link_uri)
    except ValueError:
        pass

    # Append new drone URI
    uris.append(new_uri)

    with open(CRAZYFLIE_URIS_FILENAME, 'w') as uris_file:
        json.dump({'crazyflie_uris': uris}, uris_file)
        uris_file.write('\n')
