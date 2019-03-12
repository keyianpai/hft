#!/bin/bash

cd
cd hft
#git fetch origin
#git reset --hard origin/master
#make


ssh -i ~/.ssh/ali_key test@127.0.0.1 "rm -rf ~/deploy;mkdir ~/deploy"
cd build/bin
./getins
scp -i ~/.ssh/ali_key mid_data order_proxy data_proxy ctpdata ctporder simplearb test@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key instruments.conf test@127.0.0.1:~/deploy
cd ~/hft/scripts/test
scp -i ~/.ssh/ali_key test_BuildRunEnv.sh test_stop.sh test_StartData.sh test_StartOrder.sh test_StartData_night.sh test_StartOrder_night.sh test_zip_data.sh  test_StartSimpleArb.sh test_StartSimpleArb_night.sh test@127.0.0.1:~/deploy
scp -i ~/.ssh/ali_key ~/hft/external/common/lib/libcommontools.so test@127.0.0.1:~/deploy
