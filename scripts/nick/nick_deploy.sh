#!/bin/bash

cd
cd hft
#git fetch origin
#git reset --hard origin/master
#make


ssh -i ~/.ssh/ali_key nick@127.0.0.1 "rm -rf ~/deploy;mkdir ~/deploy"
cd build/bin
./getins
scp -i ~/.ssh/ali_key mid_data order_proxy data_proxy easy_strat ctpdata ctporder strat arbmaker simplearb simplemaker nick@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key instruments.conf nick@127.0.0.1:~/deploy
cd ~/hft/scripts/nick
scp -i ~/.ssh/ali_key nick_BuildRunEnv.sh nick_stop.sh nick_StartData.sh nick_StartOrder.sh nick_StartStrat.sh nick_StartData_night.sh nick_StartOrder_night.sh nick_StartStrat_night.sh nick_StartArb.sh nick_StartArb_night.sh nick_zip_data.sh  nick_StartSimpleArb.sh nick_StartSimpleArb_night.sh nick_StartSimpleMaker.sh nick_StartSimpleMaker_night.sh nick@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key ~/hft/external/common/lib/libcommontools.so nick@127.0.0.1:~/deploy
