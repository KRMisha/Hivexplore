# Common build/runtime dependencies stage
FROM ubuntu:20.04 AS dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    freeglut3 \
    libfreeimage3 \
    libfreeimageplus3 \
    liblua5.3-0 \
    libxmu6 \
    qt5-default

# Build stage
FROM dependencies AS build

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    asciidoc \
    build-essential \
    cmake \
    doxygen \
    freeglut3-dev \
    git \
    graphviz \
    graphviz-dev \
    libfreeimage-dev \
    libfreeimageplus-dev \
    liblua5.3-dev \
    libxi-dev \
    libxmu-dev

# Build and install ARGoS from source
RUN git clone https://github.com/KRMisha/argos3 && \
    cd argos3 && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DARGOS_INSTALL_LDSOCONF=OFF ../src && \
    make -j$(nproc)
RUN cd argos3/build && \
    make doc && \
    make install

# Base stage
FROM dependencies AS base
COPY --from=build /usr/local /usr/local
RUN echo /usr/local/lib/argos3 > /etc/ld.so.conf.d/argos3.conf && ldconfig
WORKDIR /hivexplore

# Dev container stage
FROM base AS devcontainer
RUN apt-get update && apt-get install -y \
    build-essential \
    clang-format \
    cmake \
    freeglut3-dev \
    git \
    liblua5.3-dev

# Production simulation build stage
FROM base as build-prod
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    freeglut3-dev \
    liblua5.3-dev

# Build simulation
COPY simulation .
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Production stage
FROM base AS prod
COPY --from=build-prod /hivexplore .
CMD [ "argos3", "-c", "experiments/hivexplore.argos" ]

# TODO: Remove the useless simulation/ subdir
# TODO: Multi-stage production stage to build simulation

# TODO: Check if more lightweight distro than Ubuntu works
# TODO: --no-install-recommends
# TODO: Check if clang is necessary for clang-format

# TODO: Rename Makefile targets for Docker (no -prod, rather -dev and no suffix for "prod")
