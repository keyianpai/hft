#ifndef SRC_BACKTEST_ORDER_HANDLER_H_
#define SRC_BACKTEST_ORDER_HANDLER_H_

#include <order_side.h>
#include <order.h>
#include <stdio.h>
#include <Python.h>
#include <tr1/unordered_map>
#include <common_tools.h>
#include <sender.h>
#include <exchange_info.h>

#include <libconfig.h++>

#include <stdexcept>
#include <string>

class OrderHandler {
 public:
  OrderHandler();
  ~OrderHandler();

  bool Handle(const Order & order);

 private:
  void HandleConfig(const Order & order);
  void HandleContractConfig(const Order & order);
  void HandleNew(const Order & order);
  // void HandleCancel(const Order & order);
  // void HandleMod(const Order & order);
  void HandleDate(std::string s);
  void HandleLeft();
  void GenBackTestReport();
  void GenDayReport();
  void Clear();

  void Plot();

  void SendFakeFilledInfoBack(const Order & order);

  std::tr1::unordered_map<std::string, double> fozen_capital_map;

  std::tr1::unordered_map<std::string, std::string> contract_strat_map;

  std::tr1::unordered_map<std::string, double> avgcost_map;
  std::tr1::unordered_map<std::string, double> realized_pnl_map;
  std::tr1::unordered_map<std::string, int> current_pos_map;
  std::tr1::unordered_map<std::string, double> strat_pnl_map;
  std::tr1::unordered_map<std::string, int> close_size_map;
  std::tr1::unordered_map<std::string, int> force_close_map;
  std::tr1::unordered_map<std::string, double> fee_map;

  std::tr1::unordered_map<std::string, double> cum_avgcost_map;
  std::tr1::unordered_map<std::string, double> cum_realized_pnl_map;
  std::tr1::unordered_map<std::string, int> cum_left_pos_map;
  std::tr1::unordered_map<std::string, double> cum_strat_pnl_map;
  std::tr1::unordered_map<std::string, int> cum_close_size_map;
  std::tr1::unordered_map<std::string, int> cum_force_close_map;
  std::tr1::unordered_map<std::string, double> cum_fee_map;

  std::tr1::unordered_map<std::string, int> sum_left_map;

  std::tr1::unordered_map<std::string, double> deposit_rate_map;
  std::tr1::unordered_map<std::string, double> open_fee_rate_map;
  std::tr1::unordered_map<std::string, double> close_today_rate_map;
  std::tr1::unordered_map<std::string, double> close_rate_map;
  std::tr1::unordered_map<std::string, double> min_price_move_map;
  std::tr1::unordered_map<std::string, int> contract_size_map;
  FILE* record_file;
  std::string date;
  std::string main_ticker;
  std::string hedge_ticker;
  Sender * sender;
};

#endif  // SRC_BACKTEST_ORDER_HANDLER_H_
