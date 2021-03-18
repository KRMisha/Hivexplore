# Base stage
FROM node:14-alpine as base
WORKDIR /hivexplore
COPY package*.json ./
RUN npm ci
COPY . .

# Build stage
FROM base as build
RUN npm run build

# Production stage
FROM nginx:alpine as prod
WORKDIR /
COPY --from=build /hivexplore/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/nginx.conf
