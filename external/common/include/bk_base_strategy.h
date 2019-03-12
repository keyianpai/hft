#ifndef BASE_STRATEGY_H_
#define BASE_STRATEGY_H_

#include "market_snapshot.h"
#include "order.h"
#include "sender.h"
#include "exchange_info.h"
#include "order_status.h"
#include "common_tools.h"
#include "strategy_status.h"
#include <tr1/unordered_map>

#include <cmath>
#include <vector>
#include <string>
#include <unistd.h>

#include "simtrade/strat/timecontroller.h"

class BaseStrategy {
 public:
  BaseStrategy();
      
  virtual ~BaseStrategy() {}

  virtual void Start() = 0;
  virtual void Stop() = 0;
  void UpdateData(MarketSnapshot shot);
  void UpdateExchangeInfo(ExchangeInfo info);
  void RequestQryPos();
  virtual void Print();
 protected:
  void UpdateAvgCost(std::string contract, double trade_price, int size);
  std::string GenOrderRef();
  void NewOrder(std::string contract, OrderSide::Enum side, int size, bool control_price, bool sleep_order, std::string tbd = "null");
  void ModOrder(Order* o, bool sleep=false);
  void CancelAll(std::string contract);
  void CancelAll();
  void CancelOrder(Order* o);
  void DelOrder(std::string ref);
  void DelSleepOrder(std::string ref);
  void ClearAll();
  void Wakeup();
  void Wakeup(Order* o);
  void UpdatePos(Order* o, ExchangeInfo info);
  virtual void ModerateOrders(std::string contract, double edurance = 0.0);
  virtual bool PriceChange(double current_price, double reasonable_price, OrderSide::Enum side, double edurance);
  virtual void DoOperationAfterUpdatePos(Order* o, ExchangeInfo info);
  virtual void DoOperationAfterUpdateData(MarketSnapshot shot);
  virtual void DoOperationAfterFilled(Order* o, ExchangeInfo info);
  virtual void DoOperationAfterCancelled(Order* o);

  virtual double OrderPrice(std::string contract, OrderSide::Enum side, bool control_price) = 0;
  
  bool position_ready;
  bool is_started;
  Sender* sender;
  tr1::unordered_map<std::string, MarketSnapshot> shot_map;
  tr1::unordered_map<std::string, Order*> order_map;
  tr1::unordered_map<std::string, Order*> sleep_order_map;
  tr1::unordered_map<std::string, int> position_map;
  tr1::unordered_map<std::string, double> avgcost_map;
  int ref_num;
  pthread_mutex_t cancel_mutex;
  pthread_mutex_t order_ref_mutex;
  pthread_mutex_t mod_mutex;
  FILE* order_file;
  FILE* exchange_file;
  bool e_s;
  bool e_f;
  std::string m_strat_name;
  // TimeController m_tc;
  int m_contract_size;
  std::tr1::unordered_map<std::string, int> cancel_map;
  StrategyStatus::Enum ss;
};

#endif  // BASE_STRATEGY_H_
