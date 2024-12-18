# Base stage
FROM python:3.8-slim AS base
WORKDIR /hivexplore
RUN apt-get update && apt-get install -y libusb-1.0-0
COPY requirements requirements
RUN pip install -r requirements/common.txt

# Dev stage
FROM base AS dev
RUN pip install -r requirements/dev.txt
COPY . .

# Format stage
FROM dev AS format
RUN yapf --diff --parallel --recursive .

# Lint stage
FROM dev AS lint
RUN pylint server tests && mypy server tests

# Test stage
FROM dev AS test
RUN pytest

# Production stage
FROM base AS prod
RUN pip install -r requirements/prod.txt
COPY server server
COPY config config
EXPOSE 5678
ENTRYPOINT ["python3", "-u", "-m", "server.main"]
