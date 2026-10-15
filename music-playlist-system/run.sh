#!/bin/sh
# Linux / macOS: build + run.  Usage: ./run.sh [port]
cd "$(dirname "$0")" || exit 1
make -C backend || exit 1
exec ./backend/musicms -p "${1:-8080}" -w frontend -d data
