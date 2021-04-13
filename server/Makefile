VENV := ../.venv

.PHONY: venv run-drone run-argos test lint typecheck format help

venv:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install -r requirements/dev.txt

run-drone:
	python3 -m server.main drone

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
	  run-drone    Run program to connect with Crazyflies\n\
	  run-argos    Run program to connect with ARGoS\n\
	  test         Test program\n\
	  lint         Lint code with pylint\n\
	  typecheck    Type-check code with mypy\n\
	  format       Format code with yapf\n"
