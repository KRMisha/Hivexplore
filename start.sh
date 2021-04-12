#!/usr/bin/env bash

# Shell script necessary to initialize x11docker before starting containers
# with Docker Compose. See https://github.com/mviereck/x11docker/issues/227

set -o errexit
set -o nounset
set -o pipefail

function exit_handler {
    if [ "$must_cleanup_x11docker" = true ]; then
        echo "Cleaning up x11docker"
        x11docker --cleanup --quiet
    fi
}

must_cleanup_x11docker=false
trap exit_handler EXIT

function print_usage {
    echo "Usage: $(basename $0) [-h] {drone|argos|build}"
}

function print_help {
    print_usage
    echo
    echo "Hivexplore - mapping rooms with drone swarms!"
    echo
    echo "Command:"
    echo "  drone        use the Crazyradio to connect to Crazyflies"
    echo "  argos        use ARGoS to simulate the drones"
    echo "  build        rebuild all Docker images"
}

if [ "$#" -ne 1 ]; then
    echo "Incorrect number of arguments"
    print_usage
    exit 1
fi

case "$1" in
    -h|--help) print_help; exit;;
    drone|argos|build) mode="$1";;
    *) echo "Unknown argument '$1'"; print_usage; exit 1;;
esac

function initialize_x11docker {
    must_cleanup_x11docker=true

    echo "Initializing x11docker"
    read x_env < <(x11docker --hostdisplay --showenv --quiet)
    export $x_env
}

echo "Running Docker Compose"

readonly BASE_FILE="docker-compose.yml"
readonly DRONE_OVERRIDE_FILE="docker-compose.drone.yml"
readonly ARGOS_OVERRIDE_FILE="docker-compose.argos.yml"

case "$mode" in
    drone) docker-compose -f "$BASE_FILE" -f "$DRONE_OVERRIDE_FILE" up;;
    argos) initialize_x11docker; docker-compose -f "$BASE_FILE" -f "$ARGOS_OVERRIDE_FILE" up;;
    build) docker-compose -f "$BASE_FILE" -f "$DRONE_OVERRIDE_FILE" -f "$ARGOS_OVERRIDE_FILE" build;;
esac
