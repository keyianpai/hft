#include <iostream>
#include <string>
#include <vector>

#include "simplemaker/strategy.h"

Strategy::Strategy(std::string main_ticker, std::string hedge_ticker, int maxpos, double tick_size, TimeController tc, int contract_size, std::string strat_name, std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map, bool enable_stdout, bool enable_file)
  : main_ticker(main_ticker),
    hedge_ticker(hedge_ticker),
    start_pos(maxpos),
    poscapital(0.0),
    min_price(tick_size),
    price_control(10.0*min_price),
    edurance(0*min_price),
    this_tc(tc),
    cancel_threshhold(3400000),
    up_diff(25),
    down_diff(-64),
    max_spread(2*min_price),
    min_train_sample(60) {
  max_pos = start_pos;
  e_s = true;
  e_f = true;
  std::string orderfile_name = strat_name + "_order.txt";
  std::string exchangefile_name = strat_name + "_exchange.txt";
  if (e_f) {
    order_file = fopen(orderfile_name.c_str(), "w");
    exchange_file = fopen(exchangefile_name.c_str(), "w");
  }
  (*ticker_strat_map)[main_ticker].push_back(this);
  (*ticker_strat_map)[hedge_ticker].push_back(this);
  (*ticker_strat_map)["positionend"].push_back(this);
  pthread_mutex_init(&add_size_mutex, NULL);
  contract_size = contract_size;
  m_strat_name = strat_name;
  MarketSnapshot shot;
  shot_map[main_ticker] = shot;
  shot_map[hedge_ticker] = shot;
  avgcost_map[main_ticker] = 0.0;
  avgcost_map[hedge_ticker] = 0.0;
}

Strategy::~Strategy() {
  delete sender;
  if (e_f) {
    fclose(order_file);
    fclose(exchange_file);
  }
}

void Strategy::Stop() {
  CancelAll(main_ticker);
  ss = StrategyStatus::Stopped;
}

void Strategy::Flatting() {
  CancelAll(main_ticker);
}

bool Strategy::IsHedged() {
  int main_pos = position_map[main_ticker];
  int hedge_pos = position_map[hedge_ticker];
  return (main_pos == -hedge_pos);
}

bool Strategy::MidSell() {
  if (mid_map[main_ticker] - mid_map[hedge_ticker] < down_diff) {
    printf("[%s %s]midsell hit, as diff id %f\n", main_ticker.c_str(), hedge_ticker.c_str(), mid_map[main_ticker] - mid_map[hedge_ticker]);
    return false;
  }
  return true;
}

bool Strategy::IsAlign() {
  if (shot_map[main_ticker].time.tv_sec == shot_map[hedge_ticker].time.tv_sec && abs(shot_map[main_ticker].time.tv_usec-shot_map[hedge_ticker].time.tv_usec) < 10000) {
    return true;
  }
  return false;
}

bool Strategy::MidBuy() {
  if (mid_map[main_ticker] - mid_map[hedge_ticker] > up_diff) {
    printf("[%s %s]midbuy hit, as diff id %f\n", main_ticker.c_str(), hedge_ticker.c_str(), mid_map[main_ticker] - mid_map[hedge_ticker]);
    return false;
  }
  return true;
}

double Strategy::CalBalancePrice() {
  int netpos = position_map[main_ticker];
  double balance_price = -1.0;
  if (netpos > 0) {  // buy pos, sell close order
    balance_price = PriceCorrector(shot_map[hedge_ticker].asks[0]+avgcost_map[main_ticker]-avgcost_map[hedge_ticker], min_price, true);
  } else if (netpos < 0) {
    balance_price = PriceCorrector(shot_map[hedge_ticker].bids[0]+avgcost_map[main_ticker]-avgcost_map[hedge_ticker], min_price);
  } else {
    printf("pos is 0 when calbalance price!\n");
    exit(1);
  }
  printf("[%s %s]Caling balance price: avg[main]=%lf avg[hedge]=%lf hedge[bid]=%lf hedge[ask]=%lf netpos = %d, balance_price=%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), avgcost_map[main_ticker], avgcost_map[hedge_ticker], shot_map[hedge_ticker].bids[0], shot_map[hedge_ticker].asks[0], netpos, balance_price);
  return balance_price;
}

bool Strategy::TradeClose(std::string contract, int size) {
  int pos = position_map[contract];
  return (pos*size <= 0);
}

void Strategy::DoOperationAfterCancelled(Order* o) {
  printf("contract %s cancel num %d!\n", o->contract, cancel_map[o->contract]);
  if (cancel_map[o->contract] > cancel_threshhold) {
    printf("contract %s hit cancel limit!\n", o->contract);
    ss = StrategyStatus::Stopped;
    Stop();
  }
}

bool Strategy::PriceChange(double current_price, double reasonable_price, OrderSide::Enum side, double edurance) {
  bool is_bilateral = true;
  if (position_map[main_ticker] > 0 && side == OrderSide::Sell) {
    is_bilateral = false;
  }
  if (position_map[main_ticker] < 0 && side == OrderSide::Buy) {
    is_bilateral = false;
  }
  if (is_bilateral) {
    if (fabs(current_price - reasonable_price) <= edurance) {
      return false;
    }
    return true;
  } else {
    if (side == OrderSide::Buy) {
      if ((current_price - reasonable_price) > 0.001) {  // revise
        return true;
      }
      if (reasonable_price - current_price > edurance) {
        return true;
      }
      return false;
    } else {
      if (reasonable_price - current_price > 0.001) {
        return true;
      }
      if (current_price - reasonable_price > edurance) {  // revise
        return true > 0;
      }
      return false;
    }
  }
}

void Strategy::AddCloseOrderSize(OrderSide::Enum side) {
  pthread_mutex_lock(&add_size_mutex);
  Order * reverse_order = NULL;
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, main_ticker.c_str())) {
      if (it->second->Valid() && it->second->side == side) {
        reverse_order = it->second;
        reverse_order->size++;
        printf("[%s %s]add close ordersize from %d -> %d\n", main_ticker.c_str(), hedge_ticker.c_str(), reverse_order->size-1, reverse_order->size);
        ModOrder(reverse_order);
      } else if (it->second->status == OrderStatus::Modifying && it->second->side == side) {
        reverse_order = it->second;
        reverse_order->size++;
        printf("[%s %s]2nd add close ordersize from %d -> %d\n", main_ticker.c_str(), hedge_ticker.c_str(), reverse_order->size-1, reverse_order->size);
      }
    }
  }
  if (reverse_order == NULL) {
    printf("not found reverse side order!119\n");
    exit(1);
  }
  pthread_mutex_unlock(&add_size_mutex);
  printf("release the lock\n");
}

double Strategy::OrderPrice(std::string contract, OrderSide::Enum side, bool control_price) {
  if (contract == hedge_ticker) {
    return (side == OrderSide::Buy)?shot_map[hedge_ticker].asks[0]:shot_map[hedge_ticker].bids[0];
  }
  // main contract
  if (control_price) {
    double return_price =  ((side == OrderSide::Buy)?shot_map[main_ticker].bids[0]-price_control:shot_map[main_ticker].asks[0]+price_control);
    printf("[%s %s]Order use price control, bid %lf, ask %lf, price control %lf, return price is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].bids[0], shot_map[main_ticker].asks[0], price_control, return_price);
    return return_price;
  }

  bool is_close = false;
  if ((position_map[main_ticker] > 0 && side == OrderSide::Sell) || (position_map[main_ticker] < 0 && side == OrderSide::Buy)) {
    is_close = true;
  }
  if (is_close && IsHedged()) {
    double balance_price = CalBalancePrice();
    fprintf(order_file, "[%s %s]close report: np is %d, hedgep is %d, avgcost hedge and main are %lf %lf, hedge ask bid is %lf %lf, main ask bid is %lf %lf, balanceprice is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), position_map[main_ticker], position_map[hedge_ticker], avgcost_map[hedge_ticker], avgcost_map[main_ticker], shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0], shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0], balance_price);
    if (side == OrderSide::Buy) {
      if (balance_price <= shot_map[main_ticker].bids[0]) {
        fprintf(order_file, "[%s %s]balance report: pricecut buy: %lf->%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].bids[0], balance_price);
        return balance_price - min_price;
      } else if (shot_map[main_ticker].bids[0] < balance_price && balance_price <= shot_map[main_ticker].asks[0]) {
        return shot_map[main_ticker].bids[0];
      } else {
        fprintf(order_file, "[%s %s]fill right now: buy: ask %lf<%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].asks[0], balance_price);
        return shot_map[main_ticker].asks[0];
      }
    } else {
      if (balance_price >= shot_map[main_ticker].asks[0]) {
        fprintf(order_file, "[%s %s]balance report: pricecut sell: %lf->%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].bids[0], balance_price);
        return balance_price + min_price;
      } else if (shot_map[main_ticker].bids[0] <= balance_price && balance_price < shot_map[main_ticker].asks[0]) {
        return shot_map[main_ticker].asks[0];
      } else {
        fprintf(order_file, "[%s %s]fill right now: sell: bid %lf>%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].bids[0], balance_price);
        return shot_map[main_ticker].bids[0];
      }
    }
  }

  if (is_close && !IsHedged()) {
    fprintf(order_file, "[%s %s]close report: np is %d, hedgep is %d, avgcost hedge and main are %lf %lf, hedge ask bid is %lf %lf, main ask bid is %lf %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), position_map[main_ticker], position_map[hedge_ticker], avgcost_map[hedge_ticker], avgcost_map[main_ticker], shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0], shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0]);
    return (side == OrderSide::Buy)?shot_map[main_ticker].bids[0]-price_control:shot_map[main_ticker].asks[0]+price_control;
  }

  return (side == OrderSide::Buy)?shot_map[main_ticker].bids[0]:shot_map[main_ticker].asks[0];
}

bool Strategy::IsParamOK() {
  double avg = 0.0;
  double std = 0.0;
  if (map_vector.size() == min_train_sample) {  // 30 min to train
    for (unsigned int i = 0; i < map_vector.size(); i++) {
      avg += map_vector[i];
    }
    avg /= map_vector.size();
    for (unsigned int i = 0; i < map_vector.size(); i++) {
      std += (map_vector[i]-avg) * (map_vector[i]-avg);
    }
    std /= map_vector.size();
    std = sqrt(std);
    up_diff = avg + 1 * std;
    down_diff = avg - 1 * std;
    printf("[%s %s] cal done,mean is %lf, std is %lf, parmeters: [%lf,%lf]\n", main_ticker.c_str(), hedge_ticker.c_str(), avg, std, down_diff, up_diff);
    return true;
  } else if (map_vector.size() > min_train_sample) {
    return true;
  } else {
    printf("[%s %s]calculating the parmeters %zu\n", main_ticker.c_str(), hedge_ticker.c_str(), map_vector.size());
    return false;
  }
}

bool Strategy::Ready() {
  if (position_ready && shot_map[main_ticker].IsGood() && shot_map[hedge_ticker].IsGood() && mid_map[main_ticker] > 10 && mid_map[hedge_ticker] > 10 && IsParamOK() && IsAlign()) {
    return true;
  }
  if (!position_ready) {
    printf("waiting position query finish!\n");
  }
  return false;
}

void Strategy::Start() {
  /*
  if (!IsHedged()) {
    printf("not hedged position, cant start!\n");
    return;
  }
  */
  int pos = position_map[main_ticker];
  if (pos != 0) {
    max_pos = abs(pos);
  }
  if (pos > 0) {
    NewOrder(main_ticker, OrderSide::Sell, pos, false, false, "closeyes");
  } else if (pos < 0) {
    NewOrder(main_ticker, OrderSide::Buy, -pos, false, false, "closeyes");
  } else {
    if (MidBuy()) {
      NewOrder(main_ticker, OrderSide::Buy, 1, false, false, "startopen");
    } else {
      NewOrder(main_ticker, OrderSide::Buy, 1, false, true, "sleepstartopen");
    }
    if (MidSell()) {
      NewOrder(main_ticker, OrderSide::Sell, 1, false, false, "startopen");
    } else {
      NewOrder(main_ticker, OrderSide::Sell, 1, false, true, "sleepstartopen");
    }
  }
}

void Strategy::DoOperationAfterUpdateData(MarketSnapshot shot) {
  if (shot.IsGood()) {
    mid_map[shot.ticker] = (shot.bids[0]+shot.asks[0]) / 2;
    if (IsAlign()) {
      printf("[%s, %s]mid_diff is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), mid_map[main_ticker]-mid_map[hedge_ticker]);
      map_vector.push_back(mid_map[main_ticker]-mid_map[hedge_ticker]);
    }
  } else {
    printf("received bad shot!\n");
    shot.Show(stdout);
    return;
  }
}

bool Strategy::Spread_Good() {
  if (shot_map[main_ticker].asks[0] - shot_map[main_ticker].bids[0] <= max_spread) {
     if (shot_map[main_ticker].asks[0] - shot_map[main_ticker].bids[0] <= max_spread) {
       return true;
     } else {
       printf("[%s %s]hedge spread too wide!%lf, %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0]);
       return false;
     }
  } else {
    printf("[%s %s]main spread too wide!%lf, %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0]);
    return false;
  }
}

void Strategy::Train() {
  /*
  MarketSnapshot shot = last_shot;
  if (shot.IsGood()) {
    mid_map[shot.ticker] = (shot.bids[0]+shot.asks[0]) / 2;
    if (IsAlign()) {
      printf("[%s, %s]mid_diff is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), mid_map[main_ticker]-mid_map[hedge_ticker]);
      map_vector.push_back(mid_map[main_ticker]-mid_map[hedge_ticker]);
    }
  } else {
    printf("received bad shot!\n");
    shot.Show(stdout);
    return;
  }
  */
}

void Strategy::Run() {
}

void Strategy::Resume() {
  Start();
}

void Strategy::Pause() {
  CancelAll(main_ticker);
}

void Strategy::InitTicker() {
  ticker_map[main_ticker] = true;
  ticker_map[hedge_ticker] = true;
  ticker_map["position_end"] = true;
}

void Strategy::InitTimer() {
  m_tc = &this_tc;
}

void Strategy::ModerateOrders(std::string contract) {
  if (contract == main_ticker) {
    ModerateOrders(main_ticker, 0);
  } else if (contract == hedge_ticker) {
    ModerateHedgeOrders();
  } else {
  }
}

void Strategy::ModerateHedgeOrders() {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, hedge_ticker.c_str())) {
      MarketSnapshot hedge_shot = shot_map[hedge_ticker];
      Order* o = it->second;
      if (o->Valid()) {
        int hedge_pos = position_map[hedge_ticker];
        if (o->side == OrderSide::Buy && fabs(o->price - hedge_shot.asks[0]) > 0.01) {
          if (hedge_pos < 0) {  // it's a close order, if need to modify, it will be a slip of price
            fprintf(order_file, "[%s %s]Slip point report:modify buy order %s: %lf->%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), o->order_ref, o->price, hedge_shot.asks[0]);
          }
          ModOrder(o);
        } else if (o->side == OrderSide::Sell && fabs(o->price - hedge_shot.bids[0]) > 0.01) {
          if (hedge_pos > 0) {
            fprintf(order_file, "[%s %s]Slip point report:modify sell order %s: %lf->%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), o->order_ref, o->price, hedge_shot.bids[0]);
          }
          ModOrder(o);
        } else {
          // TODO(nick): handle error
        }
      }
    }
  }
}

void Strategy::ModerateAllValid(std::string contract, OrderSide::Enum side) {
  printf("entering moderate all valid!\n");
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, contract.c_str())) {
      Order* o = it->second;
      if (o->Valid() && o->side == side) {
        ModOrder(o);
        return;
      } else if (o->status == OrderStatus::Sleep && o->side == side) {
        Wakeup(o);
        return;
      }
    }
  }
  printf("exiting moderate all valid!\n");
}

void Strategy::ModerateOrders(std::string contract, double edurance) {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, contract.c_str())) {
      Order* o = it->second;
      if (o->Valid()) {
        if (o->side == OrderSide::Buy && !MidBuy() && IsAlign() && position_map[main_ticker] >= 0) {  // ensure it's open
          ModOrder(o, true);  // if midbuy ok, mod to normal, else, mod to sleep
          continue;
        } else if (o->side == OrderSide::Sell && !MidSell() && IsAlign() && position_map[main_ticker] <= 0) {
          ModOrder(o, true);
          continue;
        }
        double reasonable_price = OrderPrice(contract, o->side, false);
        if (PriceChange(o->price, reasonable_price, o->side, edurance)) {
          // printf("modify order %s, price:%lf->%lf\n", o->order_ref, o->price, reasonable_price);
          ModOrder(o);
        } else {
          // printf("edure price change: from %lf->%lf, side is %s\n", o->price, reasonable_price, OrderSide::ToString(o->side));
        }
      } else if (o->status == OrderStatus::Sleep) {
        if (o->side == OrderSide::Buy && MidBuy() && IsAlign()) {
          printf("[%s %s]wake up buy orders since mid is good!%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), map_vector.back());
          shot_map[main_ticker].Show(stdout);
          shot_map[hedge_ticker].Show(stdout);
          Wakeup(o);
        } else if (o->side == OrderSide::Sell && MidSell() && IsAlign()) {
          printf("[%s %s]wake up sell orders since mid is good!%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), map_vector.back());
          shot_map[main_ticker].Show(stdout);
          shot_map[hedge_ticker].Show(stdout);
          Wakeup(o);
        }
      }
    }
  }
}

void Strategy::OpenOrder(OrderSide::Enum sd, std::string info) {  // send a open order: if not align, sleep order, if algn, sleep or not depend on the if the condition is satisfied
  std::string na = info + "notalgn";
  if (sd == OrderSide::Buy) {
    if (IsAlign()) {
      NewOrder(main_ticker, sd, 1, true, !MidBuy(), info);
    } else {
      NewOrder(main_ticker, sd, 1, true, true, na);
    }
  } else {
    if (IsAlign()) {
      NewOrder(main_ticker, sd, 1, true, !MidSell(), info);
    } else {
      NewOrder(main_ticker, sd, 1, true, true, na);
    }
  }
}

void Strategy::DoOperationAfterUpdatePos(Order* o, ExchangeInfo info) {
  int trade_size = (o->side == OrderSide::Buy)?o->traded_size:-o->traded_size;
  std::string contract = o->contract;
  int previous_pos = position_map[contract]-trade_size;
  OrderSide::Enum sd;
  OrderSide::Enum reverse_sd;
  bool is_close = TradeClose(contract, trade_size);
  if (trade_size > 0) {
    sd = OrderSide::Buy;
    reverse_sd = OrderSide::Sell;
  } else {
    sd = OrderSide::Sell;
    reverse_sd = OrderSide::Buy;
  }

  if (contract == main_ticker) {
    int main_pos = position_map[contract];
    printf("[%s %s]mainpos is %d, trade size is %d\n", main_ticker.c_str(), hedge_ticker.c_str(), main_pos, trade_size);
    if (!is_close) {  // open traded
      if (main_pos*trade_size == 1) {  // pos 0->1: cancel open order, add close order, add open order
        printf("[%s %s]opentraded and pos=1\n", main_ticker.c_str(), hedge_ticker.c_str());
        // CancelAll(main_ticker);  // cancel open
        // NewOrder(main_ticker, reverse_sd, 1, false, false, "close@size1");
        ModerateAllValid(main_ticker, reverse_sd);  // cancel and send new, if moding, nothing happen, else cancel and new, so it becomes close
      } else {  // pos > 1
        printf("[%s %s]opentraded and pos>1\n", main_ticker.c_str(), hedge_ticker.c_str());
        AddCloseOrderSize(reverse_sd);
      }
      // add open
      if (abs(main_pos) < max_pos) {
        printf("[%s %s]This order control price\n", main_ticker.c_str(), hedge_ticker.c_str());
        OpenOrder(sd, "addopen");
      }
    } else {  // close traded
      if (abs(previous_pos) == max_pos) {
        OpenOrder(reverse_sd, "makeupopenformax");  // if close traded from position of max_pos, now we have close order, we need to make up one to continue to open
      }
      if (main_pos == 0) {
        max_pos = start_pos;  // when clear pos, reinit max_pos
        printf("[%s %s]This order control price\n", main_ticker.c_str(), hedge_ticker.c_str());
        OpenOrder(sd, "addopenforpos0");
        return;
      }
    }
  } else if (contract == hedge_ticker) {
  } else {
    SimpleHandle(251);
  }
}

void Strategy::DoOperationAfterFilled(Order* o, ExchangeInfo info) {
  if (strcmp(o->contract, main_ticker.c_str()) == 0) {
    printf("[%s %s]Mid report: main_ticker's mid filled at %lf for order %s\n", main_ticker.c_str(), hedge_ticker.c_str(), info.trade_price, o->order_ref);
    fprintf(order_file, "hedge order for %s\n", o->order_ref);
    NewOrder(hedge_ticker, (o->side == OrderSide::Buy)?OrderSide::Sell : OrderSide::Buy, info.trade_size, false, false, "hedgeorder");  // hedge operation
  } else if (strcmp(o->contract, hedge_ticker.c_str()) == 0) {
    printf("[%s %s]mid report: hedge_ticker's mid filled at %lf for order %s\n", main_ticker.c_str(), hedge_ticker.c_str(), info.trade_price, o->order_ref);
  } else {
    // TODO(nick): handle error
    SimpleHandle(322);
  }
}
