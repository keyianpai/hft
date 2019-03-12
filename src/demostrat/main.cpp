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

#include "demostrat/strategy.h"

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
  }
  return NULL;
}

int main() {
  std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > ticker_strat_map;
  Recver data_recver("data_pub");
  std::vector<BaseStrategy*> sv;
  sv.push_back(new Strategy(&ticker_strat_map));
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
