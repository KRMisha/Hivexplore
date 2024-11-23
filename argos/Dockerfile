# Common ARGoS build/runtime dependencies stage
FROM ubuntu:20.04 AS dependencies
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    freeglut3 \
    libfreeimage3 \
    libfreeimageplus3 \
    liblua5.3-0 \
    libxmu6 \
    qt5-default

# ARGoS build stage
FROM dependencies AS build-argos

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
COPY --from=build-argos /usr/local /usr/local
RUN echo /usr/local/lib/argos3 > /etc/ld.so.conf.d/argos3.conf && ldconfig
WORKDIR /hivexplore

# Format stage
FROM base AS format
RUN apt-get update && apt-get install -y clang-format
COPY . .
RUN clang-format --dry-run -Werror $(find . -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.inl')

# Simulation build dependencies stage (also used for VS Code dev container)
FROM base AS simulation-build-dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang-format \
    cmake \
    freeglut3-dev \
    git \
    liblua5.3-dev

# Simulation build stage
FROM simulation-build-dependencies AS build-simulation
COPY . .
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Production stage
FROM base AS prod
COPY --from=build-simulation /hivexplore .
CMD ["argos3", "-c", "experiments/hivexplore.argos"]
