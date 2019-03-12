#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <libconfig.h++>
#include <order.h>
#include <define.h>
#include <recver.h>
#include <sender.h>
#include <market_snapshot.h>
#include <common_tools.h>
#include <base_strategy.h>
#include <sys/time.h>
#include <tr1/unordered_map>

#include <iostream>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>

#include "backtest/strategy.h"
#include "backtest/order_handler.h"

void HandleLeft() {
  return;
}

void PrintResult() {
  return;
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

void* RunOrderListener(void *param) {
  OrderHandler *oh = reinterpret_cast<OrderHandler*>(param);
  Recver recver("order_pub");
  while (true) {
    Order o;
    o = recver.Recv(o);
    if (!oh->Handle(o)) {
      printf("order failed!\n");
      o.Show(stdout);
      return NULL;
    }
  }
  return NULL;
}

int main() {
  libconfig::Config param_cfg;
  libconfig::Config contract_cfg;
  std::string param_config_path = "/root/hft/config/backtest/backtest.config";
  std::string contract_config_path = "/root/hft/config/backtest/contract.config";
  param_cfg.readFile(param_config_path.c_str());
  contract_cfg.readFile(contract_config_path.c_str());
  try {
    // const libconfig::Setting & param_root = param_cfg.getRoot();
    // const libconfig::Setting & contract_root = contract_cfg.getRoot();
    // int a = param_cfg.lookup("asd");
    // printf("%d\n", a);
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
    clock_t main_start = clock();
    std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > ticker_strat_map;
    TimeController tc(sleep_time_v, close_time_v, force_close_time_v, "data");
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
      sv.push_back(new Strategy(param_setting, contract_setting, tc, &ticker_strat_map, "test"));
    }

    printf("start exchange thread\n");
    pthread_t exchange_thread;
    if (pthread_create(&exchange_thread,
                       NULL,
                       &RunExchangeListener,
                       &ticker_strat_map) != 0) {
      perror("exchange_pthread_create");
      exit(1);
    }
    sleep(1);

    std::string matcher_mode = param_cfg.lookup("matcher_mode");
    OrderHandler oh;
    pthread_t order_thread;
    if (matcher_mode == "c++") {
      if (pthread_create(&order_thread,
                         NULL,
                         &RunOrderListener,
                         &oh) != 0) {
        perror("order_pthread_create");
        exit(1);
      }
    }
    sleep(1);
    sv.back()->SendPlainText("param_config_path", param_config_path);
    sv.back()->SendPlainText("contract_config_path", contract_config_path);
    const libconfig::Setting & file_set = param_cfg.lookup("data_file");
    int line = param_cfg.lookup("message_line");
    char buffer[SIZE_OF_SNAPSHOT];
    Sender* data_sender = new Sender("backtest_data");
    for (int i = 0; i < file_set.getLength(); i++) {
      for (auto v : sv) {
        v->Clear();
      }
      std::string file_name = file_set[i];
      printf("handling %s\n", file_name.c_str());
      sv.back()->SendPlainText("data_path", file_name);
      std::ifstream raw_file;
      int count = 0;
      clock_t start = clock();
      raw_file.open(file_name.c_str(), ios::in);
      if (!raw_file) {
        printf("%s is not existed!", file_name.c_str());
        continue;
      }
      while (!raw_file.eof()) {
        raw_file.getline(buffer, SIZE_OF_SNAPSHOT);
        MarketSnapshot shot = HandleSnapshot(buffer);
        if (!shot.IsGood()) {
          continue;
        }
        data_sender->Send(shot.Copy().c_str());
        if ((++count) % line == 0) {
          printf("line %d~%d  cost %lf second\n", count-line, count, (static_cast<double>(clock()) - start) / CLOCKS_PER_SEC);
          start = clock();
        }
        if (buffer[0] == '\0') {
          break;
        }
        shot.is_initialized = true;
        std::vector<BaseStrategy*> ticker_sv = ticker_strat_map[shot.ticker];
        for (auto v : ticker_sv) {
          v->UpdateData(shot);
        }
      }
      raw_file.close();
      sv.back()->SendPlainText("day_end", "");
    }
    printf("backtest over!\n");
    sleep(1);
    std::string legend = param_cfg.lookup("legend");
    sv.back()->SendPlainText("legend", legend);
    sleep(1);
    sv.back()->SendPlainText("backtest_end", "");
    sv.back()->SendPlainText("plot", "");
    data_sender->Send("End\n");
    printf("backtest cost %lf second\n", (static_cast<double>(clock()) - main_start) / CLOCKS_PER_SEC);
    HandleLeft();
    PrintResult();
    printf("here reached!\n");
    sleep(5);
    /*
    while (pthread_cancel(exchange_thread) != 0) {
      while (pthread_cancel(order_thread) != 0) {
      }
    }
    */
    // while (true);
  } catch(const libconfig::SettingNotFoundException &nfex) {
    printf("Setting '%s' is missing", nfex.getPath());
    exit(1);
  } catch(const libconfig::SettingTypeException &tex) {
    printf("Setting '%s' has the wrong type", tex.getPath());
    exit(1);
  } catch (const std::exception& ex) {
    printf("EXCEPTION: %s\n", ex.what());
    exit(1);
  }
  return 0;
}
