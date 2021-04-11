#!/usr/bin/env bash

# Shell script necessary to initialize x11docker before starting containers
# with Docker Compose. See https://github.com/mviereck/x11docker/issues/227

function exit_handler() {
    echo "Cleaning up x11docker"
    x11docker --cleanup --quiet
}

trap exit_handler EXIT

echo "Initializing x11docker"
read x_env < <(x11docker --hostdisplay --showenv --quiet)
export $x_env

echo "Running Docker Compose"
docker-compose -f docker-compose.yml -f docker-compose.argos.yml up

# TODO: Arguments
