#!/usr/bin/python3

import board
import adafruit_dotstar

pixels = adafruit_dotstar.DotStar(board.SCK, board.MOSI, 19 + 16 * 23, auto_write=True)
pixels.fill(0)
