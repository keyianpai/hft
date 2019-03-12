#!/bin/bash
g++ -fPIC -shared -std=c++11 -o lib/libcommontools.so include/common_tools.cpp include/base_strategy.cpp include/recver.cpp include/sender.cpp include/timecontroller.cpp -lzmq
cp lib/libcommontools.so .
