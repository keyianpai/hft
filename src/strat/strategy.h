#ifndef SRC_STRAT_STRATEGY_H_
#define SRC_STRAT_STRATEGY_H_

#include <market_snapshot.h>
#include <order.h>
#include <sender.h>
#include <exchange_info.h>
#include <order_status.h>
#include <common_tools.h>
#include <tr1/unordered_map>

#include <cmath>
#include <vector>
#include <string>

#include "strat/timecontroller.h"

class Strategy {
 public:
  Strategy(std::string main_ticker, std::string hedge_ticker, int maxpos, double tick_size, TimeController tc, int contract_size, std::string strat_name, bool enable_stdout = true, bool enable_file = true);
  ~Strategy();

  void Start();
  void Stop();
  void UpdateData(MarketSnapshot shot);
  void UpdateExchangeInfo(ExchangeInfo info);
 private:
  bool IsReady();
  void RequestQryPos();
  void NewOrder(std::string contract, OrderSide::Enum side, int size = 1, bool control_price = false);
  void ModOrder(Order* o);
  void DelOrder(std::string ref);

  double OrderPrice(std::string contract, OrderSide::Enum side, bool control_price = false);

  bool IsHedged();

  double CalBalancePrice();

  bool TradeClose(std::string contract, int size);

  void UpdateAvgCost(std::string contract, double trade_price, int size);

  bool PriceChange(double current_price, double reasonable_price, OrderSide::Enum side = OrderSide::Buy);

  void CancelAllHedge();
  void CancelAllMain();
  void ClearValidOrder();
  std::string GenOrderRef();
  void ModerateMainOrders();
  void ModerateHedgeOrders();
  void UpdatePos(Order* o, ExchangeInfo info);
  void CloseAllTodayPos();
  void AddCloseOrderSize(OrderSide::Enum side);

  bool position_ready;
  bool is_started;
  Sender* sender;
  std::string main_ticker;
  std::string hedge_ticker;
  tr1::unordered_map<std::string, MarketSnapshot> shot_map;
  tr1::unordered_map<std::string, Order*> order_map;
  tr1::unordered_map<std::string, int> position_map;
  tr1::unordered_map<std::string, double> avgcost_map;
  char order_ref[MAX_ORDERREF_SIZE];
  int ref_num;
  pthread_mutex_t order_ref_mutex;
  pthread_mutex_t add_size_mutex;
  pthread_mutex_t mod_mutex;
  int max_pos;
  FILE* order_file;
  FILE* exchange_file;
  bool e_s;
  bool e_f;
  double poscapital;
  double min_price;
  double price_control;
  double edurance;
  TimeController m_tc;
  int m_contract_size;
  std::string m_strat_name;
};

#endif  // SRC_STRAT_STRATEGY_H_
