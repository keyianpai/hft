#!/bin/bash

~/deploy/stop.sh

if [ ! -d "/running" ];then
mkdir /running
else
echo "running exiseted!"
fi

date_string=`date --rfc-3339=date`
if [ ! -d "/running/$date_string" ];then
mkdir /running/$date_string
mkdir /running/$date_string/bin
mkdir /running/$date_string/log
mkdir /running/$date_string/scripts
echo "/running/"$date_string" exiseted!"
else
mkdir /running/$date_string/bin
mkdir /running/$date_string/log
mkdir /running/$date_string/scripts
rm /running/$date_string/bin/*
rm /running/$date_string/scripts/*
fi

cd /running/$date_string

cd ~/deploy
cp -f ctpdata ctporder strat easy_strat mid_data order_proxy data_proxy /running/$date_string/bin/
cp -f BuildRunEnv.sh stop.sh  StartData.sh StartOrder.sh StartStrat.sh StartData_night.sh StartOrder_night.sh StartStrat_night.sh zip_data.sh /running/$date_string/scripts/
cp -f instruments.conf /running/$date_string
cp -f libcommontools.so /usr/local/lib

rm /today
ln -s  /running/$date_string /today

yum -y install zsh
