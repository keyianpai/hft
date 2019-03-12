#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <order.h>
#include <recver.h>
#include <sender.h>
#include <market_snapshot.h>
#include <tr1/unordered_map>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>

#include "strat_MA/strategy.h"

void HandleLeft() {
}

void PrintResult() {
}

void* RunExchangeListener(void *param) {
  Strategy* st = reinterpret_cast<Strategy*>(param);
  Recver recver("exchange_info");
  while (true) {
    ExchangeInfo info;
    info = recver.Recv(info);
    st->UpdateExchangeInfo(info);
  }
  return NULL;
}

int main() {
  std::vector<std::string> sleep_time_v;
  sleep_time_v.push_back("10:14:00-10:30:00");
  sleep_time_v.push_back("11:29:20-13:30:00");
  sleep_time_v.push_back("00:58:20-09:00:00");
  TimeController tc(sleep_time_v, "21:00:00", "14:55:30");
  Recver pd_recver("pricer_sub");
  std::vector<std::string> strat_contracts;
  std::vector<std::string> topic_v;
  strat_contracts.push_back("ni1811");
  strat_contracts.push_back("hc1810");
  strat_contracts.push_back("ni1901");
  strat_contracts.push_back("zn1808");
  strat_contracts.push_back("zn1810");
  topic_v.push_back("MA10");
  topic_v.push_back("MA30");
  Strategy s(strat_contracts, topic_v, tc, "ma");
  pthread_t exchange_thread;
  if (pthread_create(&exchange_thread,
                     NULL,
                     &RunExchangeListener,
                     &s) != 0) {
    perror("pthread_create");
    exit(1);
  }
  while (true) {
    PricerData pd;
    pd = pd_recver.Recv(pd);
    pd.Show(stdout);
    s.UpdateData(pd);
  }
  HandleLeft();
  PrintResult();
}
