# Server

## Prerequisites

- [Python >= 3.8](https://www.python.org/downloads/)
- Make

## Setup

### USB radio

In order to use the USB radio without being root, first run the following commands:
```
sudo groupadd plugdev
sudo usermod -a -G plugdev $USER
```

Log out and log back in to be a member of the `plugdev` group.

Create a file named `/etc/udev/rules.d/99-crazyradio.rules` and add the following:
```
# Crazyradio (normal operation)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="7777", MODE="0664", GROUP="plugdev"
# Bootloader
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="0101", MODE="0664", GROUP="plugdev"
```

To connect to the Crazyflie via USB, create another file named `/etc/udev/rules.d/99-crazyflie.rules` and add the following:
```
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740", MODE="0664", GROUP="plugdev"
```

Reload the udev-rules using the following commands:
```
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Virtual environment

Install the required packages with a virtual environment located at the project root:
```
make venv
```

The virtual environment must be activated once per shell session before running any command:
```
source ../.venv/bin/activate
```

To install new packages for the server, add them to one of the files in the `requirements` directory and run the following command (with the venv activated):
```
pip install -r requirements/dev.txt
```

## Usage

### Run program to connect with Crazyflies
```
make run
```

> Note: the Crazyradio PA must be connected to the computer.

### Assign a Crazyflie's address
```
python3 -m server.scripts.assign_crazyflie_address radio://0/80/2M/<current address> <new address>
```

Example:
```
python3 -m server.scripts.assign_crazyflie_address radio://0/80/2M/E7E7E7E7E7 E7E7E7E701
```

> Note: all drone addresses should start with `E7E7E7E7`, with the form `E7E7E7E7##`. The are no restrictions on the last two bytes; this allows for a maximum of 256 possible addresses.

### Set a Crazyflie's offset relative to the base

When starting a mission, the Crazyflies' offsets relative to the base must be known before takeoff.
You can change each Crazyflie's offset, in meters, by editing `server/config/crazyflies_config.json`.

The coordinate system is the following:
- X: Forward
- Y: Left
- Z: Up

All drones must be facing the same direction on mission start.

> Note: these offsets can be updated as long as the mission state is in "Standby".

### Run program to connect with the ARGoS simulation
```
make run-argos
```

### Run tests
```
make test
```

### Lint code
```
make lint
```

### Type-check code
```
make typecheck
```

### Format code
```
make format
```

### See list of available commands
```
make help
```
