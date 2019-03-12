#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <order.h>
#include <recver.h>
#include <sender.h>
#include <pricer_data.h>
#include <market_snapshot.h>
#include <tr1/unordered_map>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>

double CalEMA(std::vector<double>*ema_v, MarketSnapshot shot, double newdata_weight) {
  if (ema_v->empty()) {
    printf("First ema calculating!\n");
    return shot.last_trade;
  }
  double ema = ema_v->back() * (1 - newdata_weight) + shot.last_trade * newdata_weight;
  return ema;
}

int GetNextCalTime(int sec, int calculate_frequency) {
  int a = sec/calculate_frequency;
  return (a+1)*calculate_frequency;
}

int main() {
  int interval = 10;
  int calculate_frequency = 10;
  Recver recver("data");
  Sender sender("pricer", "connect");
  std::vector<double>ema_v;
  tr1::unordered_map<std::string, std::vector<double> >ema_map;
  tr1::unordered_map<std::string, bool>notfirst_map;
  tr1::unordered_map<std::string, MarketSnapshot>preshot_map;
  tr1::unordered_map<std::string, int>nextcaltime_map;
  tr1::unordered_map<std::string, int>seqno_map;
  std::vector<double>ma_v;
  int time_zone_diff = 8*3600;
  while (true) {
    MarketSnapshot shot;
    shot = recver.Recv(shot);
    if (!notfirst_map[shot.ticker]) {
      preshot_map[shot.ticker] = shot;
      nextcaltime_map[shot.ticker] = 9*3600;
      notfirst_map[shot.ticker] = true;
      ema_map[shot.ticker].push_back(shot.last_trade);
    }
    int sec = (shot.time.tv_sec + time_zone_diff)%(24*3600);
    int next_cal_time = nextcaltime_map[shot.ticker];
    if (sec >= next_cal_time) {
      preshot_map[shot.ticker].Show(stdout);
      PricerData pd;
      if (sec - next_cal_time > calculate_frequency) {
        int makeup_times = (sec-next_cal_time)/calculate_frequency;
        printf("ticker is %s time is %d:%d:%d, nexttime is %d:%d:%d, makeup %d times\n", shot.ticker, sec/3600, sec%3600/60, sec%60, next_cal_time/3600, next_cal_time%3600/60, next_cal_time%60, makeup_times);
        for (int i = 0; i< makeup_times; i++) {
          if (!ema_map[shot.ticker].empty()) {
            ema_map[shot.ticker].push_back(ema_map[shot.ticker].back());
          } else {
            ema_map[shot.ticker].push_back(0.0);
          }
        }
      }
      double new_ema = CalEMA(&(ema_map[shot.ticker]), preshot_map[shot.ticker], 1.0/interval*1.0);
      ema_map[shot.ticker].push_back(new_ema);
      next_cal_time = GetNextCalTime(sec, calculate_frequency);
      nextcaltime_map[shot.ticker] = next_cal_time;
      int time_sec = sec-sec%calculate_frequency;  // make it a sign
      snprintf(pd.ticker, sizeof(pd.ticker), "%s", shot.ticker);
      pd.time = shot.time;
      pd.data = new_ema;
      pd.time_sec = time_sec;
      snprintf(pd.topic, sizeof(pd.topic), "MA%d", interval);
      pd.sequence_no = seqno_map[shot.ticker]++;
      pd.shot = shot;
      sender.Send(pd);
      pd.Show(stdout);
    }
    preshot_map[shot.ticker] = shot;
  }
}
