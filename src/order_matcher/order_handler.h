#ifndef SRC_ORDER_MATCHER_ORDER_HANDLER_H_
#define SRC_ORDER_MATCHER_ORDER_HANDLER_H_

#include <order_side.h>
#include <order.h>
#include <tr1/unordered_map>
#include <common_tools.h>

#include <libconfig.h++>

#include <stdexcept>
#include <string>

class OrderHandler {
 public:
  OrderHandler();

  bool Handle(const Order & order);

 private:
  void HandleConfig(std::string s);
  void HandleNew(const Order & order);
  void HandleCancel(const Order & order);
  void HandleMod(const Order & order);
  void GenReport();
  std::tr1::unordered_map<std::string, double> fozen_capital_map;
  std::tr1::unordered_map<std::string, double> avgcost_map;
  std::tr1::unordered_map<std::string, double> realized_pnl_map;
  std::tr1::unordered_map<std::string, int> current_pos_map;
  int closed_size;
  std::tr1::unordered_map<std::string, std::string> contract_strat_map;
  std::tr1::unordered_map<std::string, double> strat_pnl_map;
};

#endif  // SRC_ORDER_MATCHER_ORDER_HANDLER_H_
