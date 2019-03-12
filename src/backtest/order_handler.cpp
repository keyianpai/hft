#include <string.h>
#include <string>
#include <fstream>

#include "backtest/order_handler.h"

OrderHandler::OrderHandler() {
  sender = new Sender("exchange_info");
}

OrderHandler::~OrderHandler() {
  delete sender;
}

bool OrderHandler::Handle(const Order & order) {
  if (order.action == OrderAction::PlainText) {
    std::string flag = order.contract;
    std::string tbd = order.tbd;
    if (flag == "param_config_path") {
      HandleConfig(order);
    } else if (flag == "contract_config_path") {
      HandleContractConfig(order);
    } else if (flag == "data_path") {
      order.Show(record_file);
      HandleDate(order.tbd);
    } else if (flag == "day_end") {
      order.Show(record_file);
      GenDayReport();
      GenBackTestReport();
    } else if (flag == "plot") {
      printf("plotting\n");
      order.Show(record_file);
      fclose(record_file);
      Plot();
      exit(1);
    } else if (flag == "backtest_end") {
      printf("here called!\n");
      order.Show(record_file);
      GenBackTestReport();
    }
  }
  if (order.action == OrderAction::NewOrder) {
    HandleNew(order);
  }
  if (order.action == OrderAction::ModOrder) {
    // HandleMod(order);
  }
  if (order.action == OrderAction::CancelOrder) {
    // HandleCancel(order);
  }
  return true;
}

void OrderHandler::Plot() {
  Py_Initialize();
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\'/root/quant/backtest\')");
  PyRun_SimpleString("import online_pnl_plot as opp");
  PyRun_SimpleString("opp.plot_pnl()");
  Py_Finalize();
}

void OrderHandler::HandleDate(std::string s) {
  date = s;
}

void OrderHandler::HandleConfig(const Order & order) {
  libconfig::Config cfg;
  std::string s = order.tbd;
  cfg.readFile(s.c_str());
  try {
    const libconfig::Setting & strategies = cfg.lookup("strategy");
    for (int i = 0; i < strategies.getLength(); i++) {
      const libconfig::Setting & setting = strategies[i];
      std::string unique_name = setting["unique_name"];
      std::string ticker1 = setting["pairs"][0];
      std::string ticker2 = setting["pairs"][1];
      main_ticker = ticker1;
      hedge_ticker = ticker2;
      contract_strat_map[ticker1] = unique_name;
      contract_strat_map[ticker2] = unique_name;
    }
    std::string output_file = cfg.lookup("backtest_out_file");
    record_file = fopen(output_file.c_str(), "w");
    char buffer[1024];
    std::ifstream config_f;
    config_f.open(s.c_str(), ios::in);
    while (!config_f.eof()) {
      config_f.getline(buffer, sizeof(buffer));
      fprintf(record_file, "%s\n", buffer);
    }
    config_f.close();
    order.Show(record_file);
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
}

void OrderHandler::HandleContractConfig(const Order & order) {
  libconfig::Config cfg;
  std::string s = order.tbd;
  cfg.readFile(s.c_str());
  try {
    const libconfig::Setting & map = cfg.lookup("map");
    for (int i = 0; i < map.getLength(); i++) {
      const libconfig::Setting & setting = map[i];
      std::string ticker = setting["ticker"];
      // min_price_move_map[ticker] = setting["min_price_move"];
      open_fee_rate_map[ticker] = setting["open_fee_rate"];
      close_today_rate_map[ticker] = setting["close_today_fee_rate"];
      close_rate_map[ticker] = setting["close_fee_rate"];
      deposit_rate_map[ticker] = setting["deposit_rate"];
      contract_size_map[ticker] = setting["contract_size"];
    }
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
}

void OrderHandler::SendFakeFilledInfoBack(const Order & order) {
  ExchangeInfo exchangeinfo;
  exchangeinfo.side = order.side;
  snprintf(exchangeinfo.contract, sizeof(exchangeinfo.contract), "%s", order.contract);
  snprintf(exchangeinfo.order_ref, sizeof(exchangeinfo.order_ref), "%s", order.order_ref);
  exchangeinfo.type = InfoType::Filled;
  exchangeinfo.trade_price = order.price;
  exchangeinfo.trade_size = order.size;
  sender->Send(exchangeinfo);
  exchangeinfo.Show(stdout);
}

void OrderHandler::HandleNew(const Order & order) {
  SendFakeFilledInfoBack(order);
  std::string ticker = order.contract;
  std::string product = contract_strat_map[order.contract];
  order.Show(record_file);
  order.Show(stdout);
  int pre_pos = current_pos_map[ticker];
  // judge close or open
  int trade_size = (order.side == OrderSide::Buy)?order.size:-order.size;
  double fee;
  current_pos_map[ticker] += trade_size;
  bool is_close = (trade_size*current_pos_map[ticker] <= 0);
  if (is_close) {
    close_size_map[product] += abs(trade_size);
    double this_pnl = (avgcost_map[ticker] - order.price) * trade_size*contract_size_map[product];
    realized_pnl_map[ticker] += this_pnl;
    strat_pnl_map[product] += this_pnl;
    if (current_pos_map[ticker] == 0) {
      avgcost_map[ticker] = 0.0;
    }
    fee = order.size*order.price*deposit_rate_map[product]*close_today_rate_map[product]*contract_size_map[product];
    printf("%s close_fee %lf,size = %d,price=%lf,deposit=%lf,close_rate=%lf,con_size=%d\n", ticker.c_str(), fee, order.size, order.price, deposit_rate_map[product], close_today_rate_map[product], contract_size_map[product]);
    fee_map[ticker] += fee;
  } else {
    avgcost_map[ticker] = (avgcost_map[ticker]*abs(pre_pos)+order.price*order.size) / abs(current_pos_map[ticker]);
    fee = order.size*order.price*deposit_rate_map[product]*open_fee_rate_map[product]*contract_size_map[product];
    printf("%s open_fee %lf,size = %d,price=%lf,deposit=%lf,close_rate=%lf,con_size=%d\n", ticker.c_str(), fee, order.size, order.price, deposit_rate_map[product], open_fee_rate_map[product], contract_size_map[product]);
    fee_map[ticker] += fee;
  }
  realized_pnl_map[ticker] -= fee;
  strat_pnl_map[product] -= fee;
  if (strcmp(order.tbd, "force_flat") == 0) {
    force_close_map[product] += abs(trade_size);
    if (!is_close) {
      printf("close open messed!\n");
      exit(1);
    }
  }
}

/*
void OrderHandler::HandleMod(const Order & order) {
  return;
}

void OrderHandler::HandleCancel(const Order & order) {
  return;
}
*/

void OrderHandler::GenDayReport() {
  fprintf(record_file, "==========================================%s==========================================\n", date.c_str());
  fprintf(record_file, "%s", MapString(strat_pnl_map, "pnl").c_str());
  fprintf(record_file, "%s", MapString(realized_pnl_map, "all pnl").c_str());
  fprintf(record_file, "%s", MapString(current_pos_map, "left_pos").c_str());
  fprintf(record_file, "%s", MapString(avgcost_map, "avgcost").c_str());
  fprintf(record_file, "%s", MapString(close_size_map, "trade size").c_str());
  fprintf(record_file, "%s", MapString(force_close_map, "force close size").c_str());
  fprintf(record_file, "%s", MapString(fee_map, "fee map").c_str());
  fprintf(record_file, "==============================================================================================\n\n");
  Clear();
}

void OrderHandler::GenBackTestReport() {
  fprintf(record_file, "==========================================BACKTEST==========================================\n");
  fprintf(record_file, "%s", MapString(cum_strat_pnl_map, "cum_pnl").c_str());
  fprintf(record_file, "%s", MapString(cum_realized_pnl_map, "cum_all pnl").c_str());
  fprintf(record_file, "%s", MapString(cum_left_pos_map, "cum_left_pos").c_str());
  fprintf(record_file, "%s", MapString(cum_avgcost_map, "cum_avgcost").c_str());
  fprintf(record_file, "%s", MapString(cum_close_size_map, "cum trade size").c_str());
  fprintf(record_file, "%s", MapString(cum_force_close_map, "cum force close size").c_str());
  fprintf(record_file, "%s", MapString(sum_left_map, "sum left size").c_str());
  fprintf(record_file, "%s", MapString(cum_fee_map, "cum_fee_map").c_str());
  fprintf(record_file, "==============================================================================================\n\n");
}

void OrderHandler::HandleLeft() {
  // update close size map and force close map
  for (auto m : close_size_map) {
    std::string con = m.first;
    cum_close_size_map[con] += m.second;
  }
  for (auto m : force_close_map) {
    std::string con = m.first;
    cum_force_close_map[con] += m.second;
  }

  for (auto m : fee_map) {
    std::string con = m.first;
    cum_fee_map[con] += m.second;
  }

  // update strat pnl and cum_pnl map
  for (auto m: realized_pnl_map) {
    std::string con = m.first;
    cum_realized_pnl_map[con] += m.second;
  }
  for (auto m: strat_pnl_map) {
    std::string con = m.first;
    cum_strat_pnl_map[con] += m.second;
  }
  // update cum_avgcost and cum_left_pos
  for (auto m : current_pos_map) {
    std::string con = m.first;
    int thisday_pos = m.second;
    int pre_pos = cum_left_pos_map[con];
    cum_left_pos_map[con] += thisday_pos;
    sum_left_map[con] += abs(thisday_pos);
    if (pre_pos * thisday_pos > 0) {  // add the left thisday_pos
      cum_avgcost_map[con] = fabs((cum_avgcost_map[con] * pre_pos + thisday_pos * avgcost_map[con]) / cum_left_pos_map[con]);
      continue;
    } else if (pre_pos* thisday_pos < 0) {  // close cum left, update pnl needed
      if (pre_pos*cum_left_pos_map[con] < 0) {  // this left close all pre left and make new direction thisday_pos
        double add_pnl = -(cum_avgcost_map[con]-avgcost_map[con]) * pre_pos;
        fprintf(record_file, "%s %d %d %lf\n", "today left close historical pos and add_pnl is ", thisday_pos, pre_pos, add_pnl);
        cum_realized_pnl_map[con] += add_pnl;
        cum_avgcost_map[con] = avgcost_map[con];
        cum_strat_pnl_map[contract_strat_map[con]] += add_pnl;
        continue;
      }
      double add_pnl = (cum_avgcost_map[con]-avgcost_map[con]) * thisday_pos;
      cum_realized_pnl_map[con] += add_pnl;
      cum_strat_pnl_map[contract_strat_map[con]] += add_pnl;
      continue;
    } else {  // thisday_pos ==0 or pre_pos == 0: just update avg
      cum_avgcost_map[con] += avgcost_map[con];
      continue;
    }
  }
}

void OrderHandler::Clear() {
  HandleLeft();
  fee_map.clear();
  fozen_capital_map.clear();
  avgcost_map.clear();
  realized_pnl_map.clear();
  current_pos_map.clear();
  strat_pnl_map.clear();
  close_size_map.clear();
  force_close_map.clear();
}
