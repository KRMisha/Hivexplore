# ARGoS

## Prerequisites

- [Docker](https://docs.docker.com/engine/install/)
- [x11docker](https://github.com/mviereck/x11docker)
- Make

## Setup

### Development

1. Copy the VS Code attached container configuration reference into your local VS Code configuration:

    ```
    make copy-config
    ```

    > To undo this at a later date, use `make clean-config`.

2. Build the Docker image for development:

    ```
    make image
    ```

### Production

1. Build the Docker image for production:

    ```
    make image-prod
    ```

## Usage

### Development - Using the VS Code Remote-Containers extension

1. Start the container non-interactively with x11docker:

    ```
    make start
    ```

2. [Attach VS Code to the running container](https://code.visualstudio.com/docs/remote/attach-container). A new VS Code window should now appear with the project already mounted in the container.

3. Once in the container, you can make your changes and re-run the simulation:

    ```
    make run
    ```

    The simulation will start. ARGoS will not show a window until the server is running and has connected to it. Once it does, the simulator window will appear.

3. When done making changes, simply close the VS Code window with the container and stop x11docker with `Ctrl+C`.

### Production - Starting the container interactively with x11docker
```
make start-prod
```

### Build simulation
```
make build
```

### Build and run simulation
```
make run
```

> This will automatically run CMake if no Makefile exists and rebuild the program if the source files have changed.

### Format code
```
make format
```

### Clean simulation build directory
```
make clean
```

### See list of available commands

```
make help
```

> Note that some commands are meant to be ran outside a container, and some once inside the development container. See the output of `make help` for more information.
