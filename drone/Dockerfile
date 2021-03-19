# Base stage
FROM ubuntu:20.04 AS base
WORKDIR /hivexplore
RUN apt-get update && apt-get install -y \
    clang-format \
    gcc-arm-none-eabi \
    git \
    make \
    python3 \
    python3-pip
COPY requirements.txt .
RUN pip3 install -r requirements.txt

# Build stage
FROM base AS build
COPY . .
RUN git init
WORKDIR /hivexplore/app_api
RUN make -j$(nproc)

# Format stage
FROM build AS format
RUN clang-format --dry-run -Werror $(find . -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.inl')

# Flash stage
FROM build AS flash
CMD [ "make", "cload" ]
