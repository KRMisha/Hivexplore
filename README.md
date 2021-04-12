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

## Production - single command startup

### Prerequisites

- Linux
- Bash
- [Docker](https://docs.docker.com/engine/install/)
- [Docker Compose](https://docs.docker.com/compose/install/)
- [x11docker](https://github.com/mviereck/x11docker)

## Development


### Usage

To open the project in VS Code, use the multi-root workspace for best results:
```
code Hivexplore.code-workspace
```

This lets you take advantage of integration with extensions, language support, debugging, formatting, linting, and settings directly within VS Code.

Alternatively, you can double-click on the `Hivexplore.code-workspace` file in your file explorer.

**See each subproject's README for more information.**
