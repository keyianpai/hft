#!/bin/bash

cd
cd hft
#git fetch origin
#git reset --hard origin/master
#make


ssh -i ~/.ssh/ali_key developer@127.0.0.1 "rm -rf ~/deploy;mkdir ~/deploy"
cd build/bin
./getins
scp -i ~/.ssh/ali_key mid_data order_proxy data_proxy easy_strat ctpdata ctporder strat arbmaker simplearb simplemaker developer@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key instruments.conf developer@127.0.0.1:~/deploy
cd ~/hft/scripts/dev
scp -i ~/.ssh/ali_key dev_BuildRunEnv.sh dev_stop.sh dev_StartData.sh dev_StartOrder.sh dev_StartStrat.sh dev_StartData_night.sh dev_StartOrder_night.sh dev_StartStrat_night.sh dev_StartArb.sh dev_StartArb_night.sh dev_zip_data.sh  dev_StartSimpleArb.sh dev_StartSimpleArb_night.sh dev_StartSimpleMaker.sh dev_StartSimpleMaker_night.sh developer@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key ~/hft/external/common/lib/libcommontools.so developer@127.0.0.1:~/deploy
