#!/bin/zsh

cd ~/today
~/today/bin/ctporder >> ~/today/log/order_night.log &!
~/today/bin/order_proxy &!
