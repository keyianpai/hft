#ifndef SRC_DEMOSTRAT_STRATEGY_H_
#define SRC_DEMOSTRAT_STRATEGY_H_

#include <market_snapshot.h>
#include <timecontroller.h>
#include <order.h>
#include <sender.h>
#include <exchange_info.h>
#include <order_status.h>
#include <common_tools.h>
#include <base_strategy.h>
#include <tr1/unordered_map>

#include <cmath>
#include <vector>
#include <string>


class Strategy : public BaseStrategy {
 public:
  Strategy(std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map);
  ~Strategy();

  // must realize
  void Start();
  void Stop();
 private:
  // not must realize, but usually, it should
  void DoOperationAfterUpdatePos(Order* o, ExchangeInfo info);
  void DoOperationAfterUpdateData(MarketSnapshot shot);
  void DoOperationAfterFilled(Order* o, ExchangeInfo info);
  void DoOperationAfterCancelled(Order* o);

  // not must
  void ModerateOrders(std::string contract);

  void InitTicker();
  void InitTimer();
  bool Ready();
  void Pause();
  void Resume();
  void Run();
  void Train();
  void Flatting();

  // must realize, define the order price logic when send an order
  double OrderPrice(std::string contract, OrderSide::Enum side, bool control_price);

  std::string main_ticker;
  std::string hedge_ticker;
};

#endif  // SRC_DEMOSTRAT_STRATEGY_H_
