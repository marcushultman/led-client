#!/bin/bash

set -eu

ssh pi@192.168.1.66

cd ~/spoticode-apa102
git pull

mkdir -p ~/spoticode-apa102/build
cd ~/spoticode-apa102/build

cmake -DPI=1 ..
make

./spoticode
