#!/bin/bash

set -eu

if [ "$#" -ne 1 ]; then
  >&2 echo "Target not provided"
  exit 1
fi

cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null

git fetch origin main
git checkout origin/main

mkdir -p build
cd build

BIN_PATH="./$1"
TARGET=$( basename $1 )

cmake -DPI=1 ..
make "$TARGET"
pkill -SIGTERM $TARGET || true

if [ ! -f "$BIN_PATH" ]; then
  >&2 echo "Executable not found"
  exit 1
fi

nohup bash -c "$BIN_PATH $@ > >(/usr/bin/logger -t $TARGET) 2> >(/usr/bin/logger -t $TARGET-err)" 1>/dev/null 2>/dev/null &

echo "Done!"
