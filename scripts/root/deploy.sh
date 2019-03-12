#!/bin/bash

cd
cd hft
#git fetch origin
#git reset --hard origin/master
#make


ssh -i ~/.ssh/ali_key root@127.0.0.1 "cd;rm -rf deploy;mkdir deploy"
cd build/bin
./getins
scp -i ~/.ssh/ali_key mid_data order_proxy data_proxy easy_strat ctpdata ctporder strat root@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key instruments.conf root@127.0.0.1:~/deploy
cd ~/hft/scripts/root
scp -i ~/.ssh/ali_key BuildRunEnv.sh stop.sh StartData.sh StartOrder.sh StartStrat.sh StartData_night.sh StartOrder_night.sh StartStrat_night.sh root@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key ~/hft/external/common/lib/libcommontools.so root@127.0.0.1:~/deploy
