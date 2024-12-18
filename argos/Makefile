CMAKE_BUILD_DIR := build

HIVEXPLORE_SOCKET_DIR := /tmp/hivexplore

LOCAL_ARGOS_VSCODE_CONFIG_DIR := $$HOME/.config/Code/User/globalStorage/ms-vscode-remote.remote-containers/imageConfigs
LOCAL_ARGOS_VSCODE_CONFIG := $(LOCAL_ARGOS_VSCODE_CONFIG_DIR)/hivexplore%2fargos%3adev.json

.PHONY: all copy-config clean-config image-dev image start-dev start cmake build run clean format help

# Default target for building
all: build

# Commands for use outside the container dev environment
copy-config:
	mkdir -p $(LOCAL_ARGOS_VSCODE_CONFIG_DIR)
	cp argos-devcontainer.json $(LOCAL_ARGOS_VSCODE_CONFIG)

clean-config:
	rm -f $(LOCAL_ARGOS_VSCODE_CONFIG)

image-dev:
	docker build -t hivexplore/argos:dev --target simulation-build-dependencies .

image:
	docker build -t hivexplore/argos .

start-dev:
	mkdir -p $(HIVEXPLORE_SOCKET_DIR)
	x11docker --hostdisplay --name hivexplore -- "-v \"$$(pwd)/..\":/hivexplore -v $(HIVEXPLORE_SOCKET_DIR):$(HIVEXPLORE_SOCKET_DIR)" hivexplore/argos:dev

start:
	mkdir -p $(HIVEXPLORE_SOCKET_DIR)
	x11docker --hostdisplay --name hivexplore -i -- "-v $(HIVEXPLORE_SOCKET_DIR):$(HIVEXPLORE_SOCKET_DIR)" hivexplore/argos

# Commands for use inside the container dev environment
cmake: $(CMAKE_BUILD_DIR)/Makefile

$(CMAKE_BUILD_DIR)/Makefile:
	mkdir -p $(CMAKE_BUILD_DIR)
	cd $(CMAKE_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug ..

build: cmake
	cd $(CMAKE_BUILD_DIR) && make -j$$(nproc)

run: build
	argos3 -c experiments/hivexplore.argos

clean:
	rm -rf $(CMAKE_BUILD_DIR)

format:
	@clang-format -i $$(find . -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.inl')

# Help
help:
	@printf "\
	Usage: make target...\n\
	\n\
	Targets:\n\
	  Commands for use outside the container dev environment:\n\
	    copy-config     Enable ARGoS VS Code attached container configuration by copying it to ~/.config\n\
	    clean-config    Remove ARGoS VS Code attached container configuration from ~/.config\n\
	    image-dev       Build the ARGoS docker image (for development)\n\
	    image           Build the ARGoS docker image with a built simulation\n\
	    start-dev       Start the ARGoS docker container non-interactively with x11docker (for development)\n\
	    start           Run the ARGoS docker container interactively with x11docker\n\
	\n\
	  Commands for use inside the container dev environment:\n\
	    cmake           Generate a Makefile with CMake\n\
	    build           Build the ARGoS simulation with the generated CMake Makefile (default target)\n\
	    run             Build and run the ARGoS simulation\n\
	    clean           Clean CMake build directory\n\
	    format          Format code with clang-format\n"
