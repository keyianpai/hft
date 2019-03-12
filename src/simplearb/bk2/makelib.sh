#!/bin/bash

g++ -o ../../../external/strategy/simplearb/lib/libsimplearb.so -shared -fPIC -I ~/hft/external/common/include -I ~/hft/external/strategy/simplearb/include strategy.cpp
#-L ~/hft/external/common/lib -lcommontools -lzmq -lconfig++
