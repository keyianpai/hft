#!/bin/bash

~/deploy/test_stop.sh

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
cp -f ctpdata ctporder mid_data order_proxy data_proxy simplearb ~/running/$date_string/bin/
cp -f test_BuildRunEnv.sh test_stop.sh  test_StartData.sh test_StartOrder.sh test_StartData_night.sh test_StartOrder_night.sh test_zip_data.sh test_StartSimpleArb.sh test_StartSimpleArb_night.sh ~/running/$date_string/scripts/
cp -f instruments.conf ~/running/$date_string
#cp -f libcommontools.so /usr/local/lib

rm ~/today
ln -s  ~/running/$date_string ~/today
