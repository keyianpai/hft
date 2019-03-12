#!/bin/bash

~/deploy/dev_stop.sh

if [ ! -d "~/running" ];then
mkdir ~/running
else
echo "running exiseted!"
fi

date_string=`date --rfc-3339=date`
if [ ! -d "~/running/$date_string" ];then
mkdir ~/running/$date_string
mkdir ~/running/$date_string/bin
mkdir ~/running/$date_string/log
mkdir ~/running/$date_string/scripts
echo "~/running/"$date_string" exiseted!"
else
mkdir ~/running/$date_string/bin
mkdir ~/running/$date_string/log
mkdir ~/running/$date_string/scripts
rm ~/running/$date_string/bin/*
rm ~/running/$date_string/scripts/*
fi

cd ~/running/$date_string

cd ~/deploy
cp -f ctpdata ctporder strat easy_strat mid_data order_proxy data_proxy arbmaker simplearb simplemaker ~/running/$date_string/bin/
cp -f dev_BuildRunEnv.sh dev_stop.sh  dev_StartData.sh dev_StartOrder.sh dev_StartStrat.sh dev_StartData_night.sh dev_StartOrder_night.sh dev_StartStrat_night.sh dev_zip_data.sh dev_StartArb.sh dev_StartArb_night.sh dev_StartSimpleArb.sh dev_StartSimpleArb_night.sh dev_StartSimpleMaker.sh dev_StartSimpleMaker_night.sh ~/running/$date_string/scripts/
cp -f instruments.conf ~/running/$date_string
#cp -f libcommontools.so /usr/local/lib

rm ~/today
ln -s  ~/running/$date_string ~/today
