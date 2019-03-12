#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <order.h>
#include <recver.h>
#include <sender.h>
#include <market_snapshot.h>
#include <common_tools.h>
#include <base_strategy.h>
#include <tr1/unordered_map>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>

#include "simplearb/strategy.h"

void HandleLeft() {
}

void PrintResult() {
}

void* RunExchangeListener(void *param) {
  std::vector<BaseStrategy*>* s_v = reinterpret_cast<std::vector<BaseStrategy*>*>(param);
  Recver recver("exchange_info");
  while (true) {
    ExchangeInfo info;
    info = recver.Recv(info);
    for (unsigned int i = 0; i < s_v->size(); i++) {
      (*s_v)[i]->UpdateExchangeInfo(info);
    }
  }
  return NULL;
}

int main() {
  std::vector<std::string> sleep_time_v;
  sleep_time_v.push_back("10:14:20-10:30:00");
  sleep_time_v.push_back("11:29:20-13:30:00");
  sleep_time_v.push_back("00:58:20-08:55:00");
  TimeController tc(sleep_time_v, "21:00:00", "14:00:30");
  Recver data_recver("data_pub");
  BaseStrategy * s1 = new Strategy("AP907", "AP905", 5, 1, tc, 10, "AP");
  BaseStrategy * s2 = new Strategy("wr1907", "wr1905", 5, 1, tc, 10, "wr");
  BaseStrategy * s3 = new Strategy("ni1905", "ni1903", 5, 10, tc, 1, "ni");
  BaseStrategy * s4 = new Strategy("cu1903", "cu1902", 5, 10, tc, 5, "cu");
  BaseStrategy * s5 = new Strategy("fu1909", "fu1905", 5, 1, tc, 10, "fu");
  BaseStrategy * s6 = new Strategy("MA905", "MA909", 5, 10, tc, 1, "MA");
  BaseStrategy * s7 = new Strategy("zn1902", "zn1903", 5, 5, tc, 5, "zn");
  BaseStrategy * s8 = new Strategy("rb1905", "rb1909", 5, 1, tc, 10, "rb");
  BaseStrategy * s9 = new Strategy("m1905", "m1903", 5, 1, tc, 10, "m");
  BaseStrategy * s10 = new Strategy("i1905", "i1909", 5, 0.5, tc, 100, "i");
  std::vector<BaseStrategy*> s_v;
  s_v.push_back(s1);
  s_v.push_back(s2);
  s_v.push_back(s3);
  s_v.push_back(s4);
  s_v.push_back(s5);
  s_v.push_back(s6);
  s_v.push_back(s7);
  s_v.push_back(s8);
  s_v.push_back(s9);
  s_v.push_back(s10);
  pthread_t exchange_thread;
  if (pthread_create(&exchange_thread,
                     NULL,
                     &RunExchangeListener,
                     &s_v) != 0) {
    perror("pthread_create");
    exit(1);
  }
  sleep(3);
  s_v.back()->RequestQryPos();
  printf("send query position ok!\n");
  while (true) {
    MarketSnapshot shot;
    shot = data_recver.Recv(shot);
    shot.is_initialized = true;
    for (unsigned int i = 0; i < s_v.size(); i++) {
      s_v[i]->UpdateData(shot);
    }
  }
  HandleLeft();
  PrintResult();
}
