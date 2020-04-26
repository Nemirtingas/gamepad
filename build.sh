#!/bin/bash

export SDKROOT=macosx10.13
clang++ -framework CoreFoundation -framework IOKit main.cpp -o test_gamepad.exe
