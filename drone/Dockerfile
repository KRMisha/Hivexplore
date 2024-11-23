# Base stage
FROM ubuntu:20.04 AS base
WORKDIR /hivexplore
RUN apt-get update && apt-get install -y \
    clang-format \
    gcc-arm-none-eabi \
    git \
    libusb-1.0-0 \
    make \
    python3 \
    python3-pip
COPY requirements.txt .
RUN pip3 install -r requirements.txt
COPY . .
RUN git init
WORKDIR /hivexplore/app_api

# Format stage
FROM base AS format
RUN clang-format --dry-run -Werror $(find . -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.inl')

# Build stage
FROM base AS build
RUN make -j$(nproc)

# Flash stage
FROM build AS flash
CMD ["make", "cload"]
