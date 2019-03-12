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
  libconfig::Config param_cfg;
  libconfig::Config contract_cfg;
  std::string config_path = "/home/test/hft/config/backtest/backtest.config";
  std::string contract_config_path = "/home/test/hft/config/backtest/contract.config";
  param_cfg.readFile(config_path.c_str());
  contract_cfg.readFile(contract_config_path.c_str());
  const libconfig::Setting &sleep_time = param_cfg.lookup("time_controller")["sleep_time"];
  const libconfig::Setting &close_time = param_cfg.lookup("time_controller")["close_time"];
  const libconfig::Setting &force_close_time = param_cfg.lookup("time_controller")["force_close_time"];
  std::vector<std::string> sleep_time_v;
  std::vector<std::string> close_time_v;
  std::vector<std::string> force_close_time_v;
  for (int i = 0; i < sleep_time.getLength(); i++) {
    sleep_time_v.push_back(sleep_time[i]);
  }
  for (int i = 0; i < close_time.getLength(); i++) {
    close_time_v.push_back(close_time[i]);
  }
  for (int i = 0; i < force_close_time.getLength(); i++) {
    force_close_time_v.push_back(force_close_time[i]);
  }
  std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > ticker_strat_map;
  TimeController tc(sleep_time_v, close_time_v, force_close_time_v);
  Recver data_recver("data_pub");
  std::vector<BaseStrategy*> sv;

  const libconfig::Setting & strategies = param_cfg.lookup("strategy");
  const libconfig::Setting & contract_setting_map = contract_cfg.lookup("map");
  std::tr1::unordered_map<std::string, int> contract_index_map;

  for (int i = 0; i < contract_setting_map.getLength(); i++) {
    const libconfig::Setting & setting = contract_setting_map[i];
    contract_index_map[setting["ticker"]] = i;
  }

  for (int i = 0; i < strategies.getLength(); i++) {
    const libconfig::Setting & param_setting = strategies[i];
    std::string con = param_setting["unique_name"];
    const libconfig::Setting & contract_setting = contract_setting_map[contract_index_map[con]];
    sv.push_back(new Strategy(param_setting, contract_setting, tc, &ticker_strat_map));
  }

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
