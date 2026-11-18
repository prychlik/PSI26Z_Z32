#!/bin/bash
set -e
cd "$(dirname "$0")/zad1b"
docker compose up --build --abort-on-container-exit
docker compose down