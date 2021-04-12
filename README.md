# Hivexplore

Mapping rooms with drone swarms!

The repo is structured as 4 subprojects:
- `client`: the web front-end
- `server`: the back-end server used to communicate with the drone swarm
- `drone`: the Crazyflie drone firmware
- `argos`: the ARGoS simulation used to test the drones

## Setup

Clone the repo with its submodules:
```
git clone --recurse-submodules https://gitlab.com/polytechnique-montr-al/inf3995/20211/equipe-102/hivexplore
```

Or, if you already cloned the repo but forgot the `--recurse-submodules` option, use the following command to fetch the submodules manually:
```
git submodule update --init --recursive
```

## Production - single-command startup

### Prerequisites

- Linux
- Bash
- [Docker](https://docs.docker.com/engine/install/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [x11docker](https://github.com/mviereck/x11docker)

### Usage

#### Crazyflie-specific startup steps

1. Setup

    1. Make sure the [USB permissions are correct](server/README.md#usb-radio) for the Crazyradio to function properly
    2. Plug in the Crazyradio PA USB dongle
    3. Make sure all the [drones are flashed](drone/README.md#flashing) with the latest version of the firmware
    4. [Assign a unique address](server/README.md#assign-a-crazyflie's-address) to each Crazyflie
    5. [Set each Crazyflie's offset relative to the base](server/README.md#set-a-crazyflie's-offset-relative-to-the-base) in the `server/server/config/crazyflies_config.json` file

2. Start Hivexplore in *drone* mode:

    ```
    ./start.sh drone
    ```

#### ARGoS-specific startup steps

1. Setup - no prior setup is needed

2. Start Hivexplore in *ARGoS* mode:

    ```
    ./start.sh argos
    ```

#### Common steps

3. Open the web client by heading to `localhost:3995` in your browser's address bar

4. Using another device on the LAN (including a mobile device)

    1. Find your host machine's local network IP address with the following command:

        ```
        ip route get 1.2.3.4 | awk '{print $7}'
        ```

    2. On your other device, head to `<ip>:3995`, replacing `<ip>` with the IP address you found in the last step

5. Shutdown

    1. Press Ctrl+C to perform a graceful shutdown of all the containers
    2. The server logs can still be read after shutting down the program by accessing their Docker volume, in `/var/lib/docker/volumes/hivexplore_logs/_data`

#### Other commands

- Rebuild containers after an update:

    ```
    ./start.sh build
    ```

- See help information:

    ```
    ./start.sh --help
    ```

## Development

### Usage

To open the project in VS Code, use the multi-root workspace for best results:
```
code Hivexplore.code-workspace
```

This lets you take advantage of integration with extensions, language support, debugging, formatting, linting, and settings directly within VS Code.

Alternatively, you can double-click on the `Hivexplore.code-workspace` file in your file explorer.

---

**See each subproject's README for more information.**
