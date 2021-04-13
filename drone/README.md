# Drone

Source code for the firmware used in the Crazyflie range of platforms, including the Crazyflie 2.X and the Roadrunner.

The Hivexplore drone logic is contained the `app_api` directory.

## Prerequisites

- ARM embedded toolchain:
    - Ubuntu 20.04+

        ```
        sudo apt install gcc-arm-none-eabi
        ```

    - Ubuntu 16.04 & 18.04:

        ```
        sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
        sudo apt update
        sudo apt install gcc-arm-embedded
        ```

    - Arch Linux

        ```
        sudo pacman -S community/arm-none-eabi-gcc community/arm-none-eabi-gdb community/arm-none-eabi-newlib
        ```

- Make sure all the [submodules have been cloned](../README.md#setup)
- [Python >= 3.8](https://www.python.org/downloads/)
- Make

## Setup

Install the required packages to flash the drone with a virtual environment located at the project root:
```
make venv
```

The virtual environment must be activated once per shell session before running `make cload`:
```
source ../.venv/bin/activate
```

To install new packages needed for the drone build process, add them to `requirements.txt` and run the following command (with the venv activated):
```
pip install -r requirements.txt
```

## Usage

### Build the Hivexplore firmware

Build the firmware with the logic defined in the `app_api` directory:
```sh
cd app_api
make
```

### Build the default (logic-less) firmware

Build the default firmware from the root of the `drone` subproject:
```sh
make
```

### Flashing

1. `cd` into the directory with the firmware you wish to flash. Most often, this will be the `app_api` directory.
2. Plug in the Crazyradio PA USB dongle
3. Put the Crazyflie in bootloader mode:
    1. Press and hold the power button
    2. When the blue LED M2 starts blinking, release the power button
    3. The blue LED M3 should now start blinking as well
4. Flash the drone using the wireless bootloader:

    ```
    make cload
    ```

> You can also flash any one of the pre-made examples contained from one of the projects in the `examples` directory.

#### Flashing with Docker

Alternatively, you can use the Docker image to flash the drone with the `app_api` firmware. This is useful if you do not want to run all the setup instructions on your computer.

1. `cd` into the `drone` directory.
2. Build the Docker image:

    ```
    docker build -t hivexplore/drone .
    ```

3. Plug in the Crazyradio PA USB dongle
4. Put the Crazyflie in bootloader mode:
    1. Press and hold the power button
    2. When the blue LED M2 starts blinking, release the power button
    3. The blue LED M3 should now start blinking as well
5. Flash the drone using the containerized wireless bootloader:

    ```
    docker run --rm --device=/dev/bus/usb hivexplore/drone
    ```

### Make targets

```
all          Shortcut for build
compile      Compile cflie.hex. WARNING: do NOT update version.c
build        Update version.c and compile cflie.elf/hex
clean_o      Clean only the object files, keep the executables (ie .elf, .hex)
clean        Clean every compiled files
mrproper     Clean every compiled files and the classical editors backup files

cload        Flash the firmware using the wireless bootloader
flash        Flash .elf using OpenOCD
halt         Halt the target using OpenOCD
reset        Reset the target using OpenOCD
openocd      Launch OpenOCD

format       Format source files
```

### Preset build options

To create custom build options, create a file named `config.mk` in the `tools/make/`
directory and fill it with options. For example:
```
PLATFORM=CF2
DEBUG=1
```

More information can be found on the
[Bitcraze documentation](https://www.bitcraze.io/documentation/repository/crazyflie-firmware/master/).

## Unit testing

### Running all unit tests

```
make unit
```

### Running one unit test

When working with one specific file, it is often convenient to run only one unit test. For example:
```
make unit FILES=test/utils/src/test_num.c
```

### Running unit tests with specific build settings

Defines are managed by Make and are passed on to the unit test code. Use the
usual way of passing arguments to Make when running tests. For instance, to run tests
for the Crazyflie 1:
```
make unit LPS_TDOA_ENABLE=1
```
