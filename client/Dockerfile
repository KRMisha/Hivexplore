# Base stage
FROM node:14-alpine AS base
WORKDIR /hivexplore
COPY package*.json ./
RUN npm ci
COPY . .

# Lint stage
FROM base AS lint
RUN npm run lint -- --no-fix --max-warnings=0

# Test stage
FROM base AS test
RUN npm run test:unit

# Build stage
FROM base AS build
RUN npm run build

# Production stage
FROM nginx:alpine AS prod
WORKDIR /
COPY --from=build /hivexplore/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/nginx.conf
