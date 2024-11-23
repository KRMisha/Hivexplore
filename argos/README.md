# ARGoS

## Prerequisites

- [Docker](https://docs.docker.com/engine/install/)
- [x11docker](https://github.com/mviereck/x11docker)
- Make

## Production

This section details how to simply run the simulation without making changes. If you want to set up the project for development to easily modify the code in a dev container, see the [development](#development) section below.

### Setup

Build the Docker image for production:

```sh
make image
```

### Usage

```sh
make start
```

> ARGoS will not show a window until the server is running and has connected to it. Once it has, the simulator window will appear.

This creates a bind mount to the host machine's `/tmp/hivexplore` directory where a Unix domain socket can be created. In this way, the containerized ARGoS simulation can communicate with the server running on the host.

This is intended to be used alongside local development commands on the host to run the server and client, rather than for *true* production. For true production, using the `start.sh` script at the root of the project is simpler, and uses a named volume (`hivexplore_socket`) rather than a bind mount.

## Development

This section details how to set up the project for rapid iteration and modification in a dev container. If you simply want to start the container to try it out without making changes, see the [production](#production) section above.

### Setup

1. Copy the VS Code attached container configuration reference into your local VS Code configuration:

    ```sh
    make copy-config
    ```

    > To undo this at a later date, use `make clean-config`.

2. Build the Docker image for development:

    ```sh
    make image-dev
    ```

### Usage

#### Open the container using the VS Code Remote-Containers extension

1. Start the container non-interactively with x11docker:

    ```sh
    make start-dev
    ```

    In a similar fashion to the production-like `make start` command, a bind mount to `/tmp/hivexplore` is created in order to communicate with the server running on the host.

2. [Attach VS Code to the running container](https://code.visualstudio.com/docs/remote/attach-container). A new VS Code window should now appear with the project already mounted in the container.

3. Once in the container, you can make your changes and re-run the simulation:

    ```sh
    make run
    ```

    The simulation will start.

    > ARGoS will not show a window until the server is running and has connected to it. Once it has, the simulator window will appear.

4. When done making changes, simply close the VS Code window with the container and stop x11docker with `Ctrl+C`.

#### Build simulation

```sh
make build
```

#### Build and run simulation

```sh
make run
```

> This will automatically run CMake if no Makefile exists and rebuild the program if the source files have changed.

#### Format code

```sh
make format
```

#### Clean simulation build directory

```sh
make clean
```

#### See list of available commands

```sh
make help
```

> Note that some commands are meant to be ran outside a container, and some once inside the development container. See the output of `make help` for more information.
