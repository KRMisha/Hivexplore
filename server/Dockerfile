# Install stage (slim-buster?)
FROM python:3.8-slim-buster as install

# Activate virtual environment
ENV VIRTUAL_ENV=../.venv
RUN python3 -m venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# Install dependencies
COPY requirements.txt .
RUN pip3 install --upgrade pip
RUN pip3 install -r requirements.txt

# Create working directory
WORKDIR /hivexplore

FROM install as production

COPY . .

EXPOSE 5678

CMD ["python3", "-m", "server.main", "argos"]
