#include <string.h>
#include <string>

#include "order_matcher/order_handler.h"

OrderHandler::OrderHandler()
  : closed_size(0) {
}

bool OrderHandler::Handle(const Order & order) {
  if (order.action == OrderAction::PlainText) {
    HandleConfig(order.tbd);
  }
  if (order.action == OrderAction::NewOrder) {
    HandleNew(order);
  }
  if (order.action == OrderAction::ModOrder) {
    HandleMod(order);
  }
  if (order.action == OrderAction::CancelOrder) {
    HandleCancel(order);
  }
  if (order.action == OrderAction::QueryPos) {
    // use it as a end flag
    GenReport();
    exit(1);
  }
  // print error message
  return true;
}

void OrderHandler::HandleConfig(std::string s) {
  libconfig::Config cfg;
  cfg.readFile(s.c_str());
  const libconfig::Setting & strategies = cfg.lookup("strategy");
  for (int i = 0; i < strategies.getLength(); i++) {
    const libconfig::Setting & setting = strategies[i];
    std::string unique_name = setting["unique_name"];
    std::string ticker1 = setting["pairs"][0];
    std::string ticker2 = setting["pairs"][1];
    contract_strat_map[ticker1] = unique_name;
    contract_strat_map[ticker2] = unique_name;
  }
}

void OrderHandler::HandleNew(const Order & order) {
  std::string ticker = order.contract;
  order.Show(stdout);
  /*
  printf("*************Start gen***************:\n");
  GenReport();
  */
  int pre_pos = current_pos_map[ticker];
  // judge close or open
  int trade_size = (order.side == OrderSide::Buy)?order.size:-order.size;
  current_pos_map[ticker] += trade_size;
  bool is_close = (trade_size*current_pos_map[ticker] <= 0);
  if (is_close) {
    closed_size += abs(trade_size);
    double this_pnl = (avgcost_map[ticker] - order.price) * trade_size;
    realized_pnl_map[ticker] += this_pnl;
    strat_pnl_map[contract_strat_map[ticker]] += this_pnl;
    if (current_pos_map[ticker] == 0) {
      avgcost_map[ticker] = 0.0;
    }
  } else {
    avgcost_map[ticker] = (avgcost_map[ticker]*abs(pre_pos)+order.price*order.size) / abs(current_pos_map[ticker]);
  }
  // GenReport();
  // printf("*************End gen***************:\n");
}
void OrderHandler::HandleMod(const Order & order) {
}
void OrderHandler::HandleCancel(const Order & order) {
}

void OrderHandler::GenReport() {
  // PrintMap(fozen_capital_map);
  printf("*****************************************allpnl*****************************************:\n");
  PrintMap(strat_pnl_map);
  printf("***************************************************************************************:\n\n");
  printf("******************************************pnl******************************************:\n");
  PrintMap(realized_pnl_map);
  printf("***************************************************************************************:\n\n");
  printf("******************************************pos******************************************:\n");
  PrintMap(current_pos_map);
  printf("***************************************************************************************:\n\n");
  printf("******************************************avg******************************************:\n");
  PrintMap(avgcost_map);
  printf("***************************************************************************************:\n\n");
  printf("tradesize is %d\n", closed_size);
}
