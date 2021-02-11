# Hivexplore

The repo is structured as 4 subprojects:
- `client`: the web front-end
- `server`: the back-end server used to communicate with the drone swarm
- `drone`: the drone firmware
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

> Note: make sure the repo is cloned to a path without spaces. Crazyflie's firmware Makefile does not work if the project path contains spaces.

## Usage

To open the repo in VS Code, use the multi-root workspace:
```
code Hivexplore.code-workspace
```

Alternatively, you can double-click on the `Hivexplore.code-workspace` file in your file explorer.

**See each subproject's README for more information.**
