#!/bin/bash

g++  -Woverloaded-virtual -Wextra -Wfloat-equal  -O2 -g -ldl -o Strategy.so -shared -fPIC -I ~/hft/external/common/include -I ~/hft/external/strategy/backtest/include -L ~/hft/external/common/lib -lcommontools -lzmq -lconfig++ wrap.h strategy.cpp -lboost_python -I/usr/include/python2.7 -I/root/anaconda2/include/python2.7
