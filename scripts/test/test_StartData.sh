#!/bin/zsh

cd ~/today
~/today/bin/ctpdata > ~/today/log/data.log &!
~/today/bin/data_proxy &!
~/today/bin/mid_data &!
