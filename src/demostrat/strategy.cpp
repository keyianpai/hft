#include <iostream>
#include <vector>
#include <string>

#include "demostrat/strategy.h"

Strategy::Strategy(std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map) {
  e_s = true;
  main_ticker = "ni1905";
  hedge_ticker = "IC1906";
  (*ticker_strat_map)[main_ticker].push_back(this);
  (*ticker_strat_map)[hedge_ticker].push_back(this);
  (*ticker_strat_map)["positionend"].push_back(this);
}

Strategy::~Strategy() {
  delete m_tc;
  fclose(order_file);
  fclose(exchange_file);
}

void Strategy::Stop() {
  CancelAll();
}

void Strategy::InitTicker() {
  ticker_map[main_ticker] = true;
  ticker_map[hedge_ticker] = true;
  ticker_map["positionend"] = true;
}

void Strategy::InitTimer() {
  std::vector<string> sleep_time_v;
  std::vector<string> close_time_v;
  std::vector<string> force_close_time_v;
  m_tc = new TimeController(sleep_time_v, close_time_v, force_close_time_v, "data");;
}

bool Strategy::Ready() {
  return true;
}

void Strategy::Pause() {
}

void Strategy::Resume() {
}

void Strategy::Run() {
}

void Strategy::Train() {
}

void Strategy::Flatting() {
}

void Strategy::DoOperationAfterCancelled(Order* o) {
  printf("contract %s cancel num %d!\n", o->contract, cancel_map[o->contract]);
  if (cancel_map[o->contract] > 100) {
    printf("contract %s hit cancel limit!\n", o->contract);
    Stop();
  }
}

double Strategy::OrderPrice(std::string contract, OrderSide::Enum side, bool control_price) {
  // this is a logic to make order use market price
  return (side == OrderSide::Buy)?shot_map[contract].asks[0]:shot_map[contract].bids[0];
}

void Strategy::Start() {
  // int pos = position_map[];
  // start with two order
  // NewOrder(, OrderSide::Buy, 1, false, false);
  NewOrder(main_ticker, OrderSide::Sell, 1000, false, false, "");
}

void Strategy::DoOperationAfterUpdateData(MarketSnapshot shot) {
  // shot.Show(stdout);
}

void Strategy::ModerateOrders(std::string contract) {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    Order* o = it->second;
    MarketSnapshot shot = shot_map[o->contract];
    if (o->Valid()) {
      if (o->side == OrderSide::Buy && fabs(o->price - shot.asks[0]) > 0.01) {
        ModOrder(o);
      } else if (o->side == OrderSide::Sell && fabs(o->price - shot.bids[0]) > 0.01) {
        ModOrder(o);
      } else {
      }
    }
  }
}

void Strategy::DoOperationAfterUpdatePos(Order* o, ExchangeInfo info) {
}

void Strategy::DoOperationAfterFilled(Order* o, ExchangeInfo info) {
}
