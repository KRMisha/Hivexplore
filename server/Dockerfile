# Base stage
FROM python:3.8-slim-buster as base
WORKDIR /hivexplore
COPY requirements requirements
RUN pip3 install -r requirements/common.txt

# Dev stage
FROM base as dev
RUN pip3 install -r requirements/dev.txt
COPY . .

# Format stage
FROM dev as format
RUN yapf --diff --parallel --recursive .

# Lint stage
FROM dev as lint
RUN pylint server tests && mypy server tests

# Test stage
FROM dev as test
RUN pytest

# Production stage
FROM base as prod
RUN pip3 install -r requirements/prod.txt
COPY server server
EXPOSE 5678
ENTRYPOINT [ "python3", "-m", "server.main" ]

# TODO: Check if dev requirements should also be frozen
