#!/bin/bash
xinput disable 10
xinput disable 11
./build/ioswitchctl -i 127.0.0.1 -d /dev/input/event10 -p 8081 -l 8081 -t bind
./build/ioswitchctl -i 127.0.0.1 -d /dev/input/event9 -p 8081 -l 8081 -t add
