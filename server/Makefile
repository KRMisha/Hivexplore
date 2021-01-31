VENV := .venv

.PHONY: venv run test lint format help

venv:
	python3 -m venv $(VENV)
	./$(VENV)/bin/pip install -r requirements.txt

run:
	./main.py

test:
	pytest

lint:
	flake8

format:
	black -l 140 .

help:
	@printf "\
	Usage: make target...\n\
	\n\
	Targets:\n\
	  venv      Create a new virtual environment\n\
	  run       Run program\n\
	  test      Test program\n\
	  lint      Lint code with pylint\n\
	  format    Format code with autopep8\n"
