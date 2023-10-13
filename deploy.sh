#!/bin/bash

set -eu

cd ~/spoticode-apa102
git fetch origin master
git checkout origin/master

pkill -SIGTERM spoticode || true
python3 scripts/clear.py

mkdir -p ~/spoticode-apa102/build
cd ~/spoticode-apa102/build

cmake -DPI=1 ..
make spoticode

nohup bash -c './spoticode > >(/usr/bin/logger -t spoticode) 2> >(/usr/bin/logger -t spotcode-err)' 1>/dev/null 2>/dev/null &

echo "Done!"
