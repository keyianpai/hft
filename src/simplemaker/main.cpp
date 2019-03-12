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

#include "simplemaker/strategy.h"

void HandleLeft() {
}

void PrintResult() {
}


void* RunExchangeListener(void *param) {
  std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > * sv_map = reinterpret_cast<std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >* >(param);
  Recver recver("exchange_info");
  while (true) {
    ExchangeInfo info;
    info = recver.Recv(info);
    std::vector<BaseStrategy*> sv = (*sv_map)[info.contract];
    for (auto v : sv) {
      v->UpdateExchangeInfo(info);
    }
    /*
    for (unsigned int i = 0; i < s_v->size(); i++) {
      (*s_v)[i]->UpdateExchangeInfo(info);
    }
    */
  }
  return NULL;
}

int main() {
  std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > ticker_strat_map;
  std::vector<std::string> sleep_time_v;
  sleep_time_v.push_back("10:14:20-10:30:00");
  sleep_time_v.push_back("11:29:20-13:30:00");
  std::vector<std::string> close_time_v;
  close_time_v.push_back("14:55:20-15:05:00");
  close_time_v.push_back("22:59:20-23:59:59");
  close_time_v.push_back("00:00:00-03:59:59");
  std::vector<std::string> force_close_time_v;
  TimeController tc(sleep_time_v, close_time_v, force_close_time_v);

  Recver data_recver("data_pub");
  std::vector<BaseStrategy*> sv;
  sv.push_back(new Strategy("ni1905", "ni1903", 5, 10, tc, 1, "ni", &ticker_strat_map));
  sv.push_back(new Strategy("cu1903", "cu1902", 5, 10, tc, 5, "cu", &ticker_strat_map));
  sv.push_back(new Strategy("fu1909", "fu1905", 5, 1, tc, 10, "fu", &ticker_strat_map));
  sv.push_back(new Strategy("MA905", "MA909", 5, 10, tc, 1, "MA", &ticker_strat_map));
  sv.push_back(new Strategy("zn1902", "zn1903", 5, 5, tc, 5, "zn", &ticker_strat_map));
  sv.push_back(new Strategy("rb1905", "rb1909", 5, 1, tc, 10, "rb", &ticker_strat_map));
  sv.push_back(new Strategy("m1905", "m1903", 5, 1, tc, 10, "m", &ticker_strat_map));
  sv.push_back(new Strategy("i1905", "i1909", 5, 0.5, tc, 100, "i", &ticker_strat_map));

  pthread_t exchange_thread;
  if (pthread_create(&exchange_thread,
                     NULL,
                     &RunExchangeListener,
                     &ticker_strat_map) != 0) {
    perror("pthread_create");
    exit(1);
  }
  sleep(3);
  sv.back()->RequestQryPos();
  printf("send query position ok!\n");
  while (true) {
    MarketSnapshot shot;
    shot = data_recver.Recv(shot);
    shot.is_initialized = true;
    std::vector<BaseStrategy*> sv = ticker_strat_map[shot.ticker];
    for (auto v : sv) {
      v->UpdateData(shot);
    }
  }
  HandleLeft();
  PrintResult();
}
