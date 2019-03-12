#!/bin/zsh

cd ~/today
~/today/bin/ctporder >> ~/today/log/order.log &!
~/today/bin/order_proxy &!
