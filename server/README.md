# Server

## Prerequisites

- [Python >= 3.8](https://www.python.org/downloads/)

## Setup

Install the required packages with a venv:

```
make venv
```

The virtual environment must be activated once per shell session before running any command:
```sh
# Unix:
source .venv/bin/activate

# Windows:
.venv\Scripts\activate.bat on Windows
```

## Usage

### Run program
```
make run
```

### Run tests
```
make test
```

### Lint code
```
make lint
```

### Type-check code
```
make typecheck
```

### Format code
```
make format
```

### See list of available commands
```
make help
```
