#!/bin/bash

set -eu

cd ~/spoticode-apa102
git fetch origin master
git checkout origin/master

mkdir -p ~/spoticode-apa102/build
cd ~/spoticode-apa102/build

cmake -DPI=1 ..
make spoticode

./spoticode
