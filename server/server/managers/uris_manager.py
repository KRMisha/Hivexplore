import json
from cflib.crazyflie.mem import MemoryElement
from cflib.crazyflie import Crazyflie
from cflib.utils.power_switch import PowerSwitch

CRAZYFLIE_URIS_FILENAME = 'server/config/crazyflie_uris.json'


def get_crazyflie_uris_from_file():
    with open(CRAZYFLIE_URIS_FILENAME, 'r+') as uris_file:
        try:
            data = json.load(uris_file)
            uris = data['crazyflie_uris']
            if len(uris) > 0:
                return uris
        except ValueError:
            pass

        DEFAULT_URIS = ['radio://0/80/2M/E7E7E7E701', 'radio://0/80/2M/E7E7E7E702']
        print('get_crazyflie_uris_from_file warning: Using default uris:', DEFAULT_URIS)
        json.dump({'crazyflie_uris': DEFAULT_URIS}, uris_file)
        uris_file.write('\n')
        return DEFAULT_URIS


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


def _data_written(crazyflie: Crazyflie, eeprom, _addr):
    eeprom.update(lambda eeprom: _data_updated(crazyflie, eeprom))


def _data_updated(crazyflie: Crazyflie, eeprom):
    old_address = crazyflie.link_uri.split('/')[-1]
    new_address = format(eeprom.elements['radio_address'], 'X')
    new_uri = crazyflie.link_uri.replace(old_address, new_address)

    print(f'Changed address of drone {crazyflie.link_uri} to {new_uri}')
    print('Restarting drone...')

    PowerSwitch(crazyflie.link_uri).stm_power_cycle()

    uris = get_crazyflie_uris_from_file()

    # Remove old drone URI
    try:
        uris.remove(crazyflie.link_uri)
    except ValueError:
        pass

    # Append the new one
    uris.append(new_uri)

    with open(CRAZYFLIE_URIS_FILENAME, 'w+') as uris_file:
        json.dump({'crazyflie_uris': uris}, uris_file)
        uris_file.write('\n')
