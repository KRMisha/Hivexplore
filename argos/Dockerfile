# Install stage
FROM ubuntu:20.04 AS install

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    cmake \
    sudo \
    libfreeimage-dev libfreeimageplus-dev \
    qt5-default \
    freeglut3-dev \
    libxi-dev \
    libxmu-dev \
    lua5.3 liblua5.3-dev \
    doxygen \
    graphviz graphviz-dev \
    asciidoc

# Build ARGoS from source
RUN git clone https://github.com/MISTLab/argos3.git && \
    cd argos3 && \
    git checkout inf3995 && \
    mkdir build_simulator && cd build_simulator && \
    cmake -DCMAKE_BUILD_TYPE=Release ../src && \
    make -j$(nproc)

# Install ARGoS
RUN cd argos3/build_simulator && \
    make doc && \
    make install && \
    . ./argos_post_install.sh

WORKDIR /hivexplore

# Production stage
FROM install as production

COPY simulation .

# Build simulation
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    make -j$(nproc)

CMD ["argos3", "-c", "experiments/hivexplore.argos"]
