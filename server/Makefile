VENV := ../.venv

.PHONY: venv run run-argos test lint typecheck format help

venv:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install -r requirements.txt

run:
	python3 -m server.main

run-argos:
	python3 -m server.main argos

test:
	pytest

lint:
	pylint server tests

typecheck:
	mypy server tests

format:
	yapf --in-place --parallel --recursive .

help:
	@printf "\
	Usage: make target...\n\
	\n\
	Targets:\n\
	  venv         Create a new virtual environment\n\
	  run          Run program\n\
	  test         Test program\n\
	  lint         Lint code with pylint\n\
	  typecheck    Type-check code with mypy\n\
	  format       Format code with yapf\n"
