# Install stage
FROM python:3.8-slim-buster as install

WORKDIR /hivexplore

COPY requirements/prod.txt .
RUN pip3 install -r requirements/prod.txt

# Production stage
FROM install as production

COPY . .

EXPOSE 5678

# TODO: Provide argument
CMD [ "python3", "-m", "server.main", "argos" ]
