#include <iostream>
#include <string>

#include "strat/strategy.h"

Strategy::Strategy(std::string main_ticker, std::string hedge_ticker, int maxpos, double tick_size, TimeController tc, int contract_size, std::string strat_name, bool enable_stdout, bool enable_file)
  : position_ready(false),
    is_started(false),
    main_ticker(main_ticker),
    hedge_ticker(hedge_ticker),
    ref_num(0),
    max_pos(maxpos),
    e_s(enable_stdout),
    e_f(enable_file),
    poscapital(0.0),
    min_price(tick_size),
    price_control(10.0*min_price),
    edurance(1.0*min_price),
    m_tc(tc),
    m_contract_size(contract_size),
    m_strat_name(strat_name) {
  if (e_f) {
    order_file = fopen("order.txt", "w");
    exchange_file = fopen("exchange.txt", "w");
  }
  pthread_mutex_init(&order_ref_mutex, NULL);
  pthread_mutex_init(&add_size_mutex, NULL);
  pthread_mutex_init(&mod_mutex, NULL);
  MarketSnapshot shot;
  shot_map[main_ticker] = shot;
  shot_map[hedge_ticker] = shot;
  avgcost_map[main_ticker] = 0.0;
  avgcost_map[hedge_ticker] = 0.0;
  sender = new Sender("order");
  RequestQryPos();
}

Strategy::~Strategy() {
  delete sender;
  if (e_f) {
    fclose(order_file);
    fclose(exchange_file);
  }
}

bool Strategy::IsHedged() {
  int main_pos = position_map[main_ticker];
  int hedge_pos = position_map[hedge_ticker];
  return (main_pos == -hedge_pos);
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
  printf("Caling balance price: avg[main]=%lf avg[hedge]=%lf hedge[bid]=%lf hedge[ask]=%lf netpos = %d, balance_price=%lf\n", avgcost_map[main_ticker], avgcost_map[hedge_ticker], shot_map[hedge_ticker].bids[0], shot_map[hedge_ticker].asks[0], netpos, balance_price);
  return balance_price;
}

bool Strategy::TradeClose(std::string contract, int size) {
  int pos = position_map[contract];
  return (pos*size <= 0);
}

void Strategy::UpdateAvgCost(std::string contract, double trade_price, int size) {
  double capital_change = trade_price*size;
  if (contract != main_ticker && contract != hedge_ticker) {
    printf("update avgcost contract not found %s\n", contract.c_str());
    exit(1);
  }
  int current_pos = position_map[contract];
  int pre_pos = current_pos - size;
  avgcost_map[contract] = (avgcost_map[contract] * pre_pos + capital_change)/current_pos;
}

bool Strategy::PriceChange(double current_price, double reasonable_price, OrderSide::Enum side) {
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
      if (current_price > reasonable_price) {
        return true;
      }
      if (reasonable_price - current_price > edurance) {
        return true;
      }
      return false;
    } else {
      if (current_price < reasonable_price) {
        return true;
      }
      if (current_price - reasonable_price > edurance) {
        return true;
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
        printf("add close ordersize from %d -> %d\n", reverse_order->size-1, reverse_order->size);
        ModOrder(reverse_order);
      } else if (it->second->status == OrderStatus::Modifying && it->second->side == side) {
        reverse_order = it->second;
        reverse_order->size++;
        printf("2nd add close ordersize from %d -> %d\n", reverse_order->size-1, reverse_order->size);
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
  bool is_close = false;
  if ((position_map[main_ticker] > 0 && side == OrderSide::Sell) || (position_map[main_ticker] < 0 && side == OrderSide::Buy)) {
    is_close = true;
  }
  if (is_close && IsHedged()) {
    double balance_price = CalBalancePrice();
    fprintf(order_file, "close report: np is %d, hedgep is %d, avgcost hedge and main are %lf %lf, hedge ask bid is %lf %lf, main ask bid is %lf %lf, balanceprice is %lf\n", position_map[main_ticker], position_map[hedge_ticker], avgcost_map[hedge_ticker], avgcost_map[main_ticker], shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0], shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0], balance_price);
    if (side == OrderSide::Buy) {
      if (balance_price <= shot_map[main_ticker].bids[0]) {
        fprintf(order_file, "balance report: pricecut buy: %lf->%lf\n", shot_map[main_ticker].bids[0], balance_price);
        return balance_price - min_price;
      } else if (shot_map[main_ticker].bids[0] < balance_price && balance_price <= shot_map[main_ticker].asks[0]) {
        return shot_map[main_ticker].bids[0];
      } else {
        return shot_map[main_ticker].asks[0];
      }
    } else {
      if (balance_price >= shot_map[main_ticker].asks[0]) {
        fprintf(order_file, "balance report: pricecut sell: %lf->%lf\n", shot_map[main_ticker].bids[0], balance_price);
        return balance_price + min_price;
      } else if (shot_map[main_ticker].bids[0] <= balance_price && balance_price < shot_map[main_ticker].asks[0]) {
        return shot_map[main_ticker].asks[0];
      } else {
        return shot_map[main_ticker].bids[0];
      }
    }
  }

  if (is_close && !IsHedged()) {
    fprintf(order_file, "close report: np is %d, hedgep is %d, avgcost hedge and main are %lf %lf, hedge ask bid is %lf %lf, main ask bid is %lf %lf\n", position_map[main_ticker], position_map[hedge_ticker], avgcost_map[hedge_ticker], avgcost_map[main_ticker], shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0], shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0]);
    return (side == OrderSide::Buy)?shot_map[main_ticker].bids[0]-price_control:shot_map[main_ticker].asks[0]+price_control;
  }

  // is open and position not zero, so it's a add position operation, to avoid add to max, add control
  if (control_price) {
    return (side == OrderSide::Buy)?shot_map[main_ticker].bids[0]-price_control:shot_map[main_ticker].asks[0]+price_control;
  }

  return (side == OrderSide::Buy)?shot_map[main_ticker].bids[0]:shot_map[main_ticker].asks[0];
}

std::string Strategy::GenOrderRef() {
  char orderref[32];
  snprintf(orderref, sizeof(orderref), "%s%d", m_strat_name.c_str(), ref_num++);
  return orderref;
}

void Strategy::NewOrder(std::string contract, OrderSide::Enum side, int size, bool control_price) {
    if (size == 0) {
      return;
    }
    pthread_mutex_lock(&order_ref_mutex);
    Order* order = new Order();
    snprintf(order->contract, sizeof(order->contract), "%s", contract.c_str());
    order->price = OrderPrice(contract, side, control_price);
    order->size = size;
    order->side = side;
    snprintf(order->order_ref, sizeof(order->order_ref), "%s", GenOrderRef().c_str());
    order->action = OrderAction::NewOrder;
    order->status = OrderStatus::SubmitNew;
    if (e_s) {
      order->Show(stdout);
    }
    if (e_f) {
      order->Show(order_file);
    }
    pthread_mutex_unlock(&order_ref_mutex);
    sender->Send(*order);
    order_map[order->order_ref] = order;
}

void Strategy::ModOrder(Order* o) {
  printf("modorder lock\n");
  pthread_mutex_lock(&mod_mutex);
  o->price = OrderPrice(o->contract, o->side);
  o->status = OrderStatus::Modifying;
  o->action = OrderAction::ModOrder;
  pthread_mutex_unlock(&mod_mutex);
  printf("release modorder lock\n");
  if (e_s) {
    o->Show(stdout);
  }
  if (e_f) {
    o->Show(order_file);
  }
  sender->Send(*o);
}

void Strategy::Start() {
  int pos = position_map[main_ticker];
  if (pos > 0) {
    NewOrder(main_ticker, OrderSide::Sell, pos);
    if (pos < max_pos) {
      NewOrder(main_ticker, OrderSide::Buy);
    }
  } else if (pos < 0) {
    NewOrder(main_ticker, OrderSide::Buy, -pos);
    if (-pos < max_pos) {
      NewOrder(main_ticker, OrderSide::Sell);
    }
  } else {
    NewOrder(main_ticker, OrderSide::Buy);
    NewOrder(main_ticker, OrderSide::Sell);
  }
}

void Strategy::ModerateMainOrders() {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, main_ticker.c_str())) {
      Order* o = it->second;
      if (o->Valid()) {
        double reasonable_price = OrderPrice(main_ticker, o->side);
        if (!DoubleEqual(o->price, reasonable_price) && !PriceChange(o->price, reasonable_price, o->side)) {
          printf("edure price change: from %lf->%lf, side is %s\n", o->price, reasonable_price, OrderSide::ToString(o->side));
        }
        if (o->side == OrderSide::Buy && PriceChange(o->price, reasonable_price, o->side)) {
          printf("modify order %s, price:%lf->%lf\n", o->order_ref, o->price, reasonable_price);
          ModOrder(o);
        } else if (o->side == OrderSide::Sell && PriceChange(o->price, reasonable_price, o->side)) {
          printf("modify order %s, price:%lf->%lf\n", o->order_ref, o->price, reasonable_price);
          ModOrder(o);
        } else {
          // TODO(nick): handle error
        }
      }
    }
  }
}

void Strategy::ModerateHedgeOrders() {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, hedge_ticker.c_str())) {
      Order* o = it->second;
      if (o->Valid()) {
        if (o->side == OrderSide::Buy && !DoubleEqual(o->price, shot_map[hedge_ticker].asks[0])) {
          fprintf(order_file, "Slip point report:modify buy order %s: %lf->%lf\n", o->order_ref, o->price, shot_map[hedge_ticker].asks[0]);
          ModOrder(o);
        } else if (o->side == OrderSide::Sell && !DoubleEqual(o->price, shot_map[hedge_ticker].bids[0])) {
          fprintf(order_file, "Slip point report:modify sell order %s: %lf->%lf\n", o->order_ref, o->price, shot_map[hedge_ticker].asks[0]);
          ModOrder(o);
        } else {
          // TODO(nick): handle error
        }
      }
    }
  }
}

void Strategy::CloseAllTodayPos() {
  int main_pos = position_map[main_ticker];
  OrderSide::Enum main_side = (main_pos > 0)?OrderSide::Sell:OrderSide::Buy;
  int hedge_pos = position_map[hedge_ticker];
  OrderSide::Enum hedge_side = (hedge_pos > 0)?OrderSide::Sell:OrderSide::Buy;

  pthread_mutex_lock(&order_ref_mutex);
  Order* main_order = new Order();
  snprintf(main_order->contract, sizeof(main_order->contract), "%s", main_ticker.c_str());
  main_order->price = (main_side == OrderSide::Buy)?shot_map[main_ticker].asks[0]+5*min_price:shot_map[main_ticker].bids[0]-5*min_price;
  main_order->size = abs(main_pos);
  main_order->side = main_side;
  snprintf(main_order->order_ref, sizeof(main_order->order_ref), "%s", GenOrderRef().c_str());
  main_order->action = OrderAction::NewOrder;
  main_order->status = OrderStatus::SubmitNew;
  if (e_s) {
    main_order->Show(stdout);
  }
  if (e_f) {
    main_order->Show(order_file);
  }
  pthread_mutex_unlock(&order_ref_mutex);
  sender->Send(*main_order);

  NewOrder(hedge_ticker, hedge_side, abs(hedge_pos));

  printf("close all:position is %d %d avgcost is %lf %lf, market price is %lf %lf\n", main_pos, hedge_pos, avgcost_map[main_ticker], avgcost_map[hedge_ticker], main_order->price, OrderPrice(hedge_ticker, hedge_side));
}

void Strategy::UpdateData(MarketSnapshot shot) {
  if (!m_tc.TimeValid() && is_started) {
    printf("time to sleep!\n");
    ClearValidOrder();
    sleep(3);
    ClearValidOrder();  // make sure no orders
    is_started = false;
    if (m_tc.TimeClose()) {
      CloseAllTodayPos();
    }
    return;
  }
  if (m_tc.TimeValid() && IsReady() && !is_started) {
    printf("time to wake up!\n");
    Start();
    is_started = true;
    return;
  }
  std::string shot_ticker = shot.ticker;
  if (shot_ticker == main_ticker) {
    shot_map[main_ticker] = shot;
    ModerateMainOrders();
  } else if (shot_ticker == hedge_ticker) {
    shot_map[hedge_ticker] = shot;
    ModerateHedgeOrders();
  } else {
    // SimpleHandle(167);
  }
}

void Strategy::RequestQryPos() {
  position_map.clear();
  Order* o = new Order();
  o->action = OrderAction::QueryPos;
  sender->Send(*o);
}

bool Strategy::IsReady() {
  if (shot_map[main_ticker].is_initialized && shot_map[hedge_ticker].is_initialized && position_ready) {
    return true;
  }
  if (!position_ready) {
    printf("waiting position query finish!\n");
  }
  return false;
}

void Strategy::CancelAllMain() {
  printf("Enter Cancel ALL Main\n");
  pthread_mutex_lock(&mod_mutex);
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, main_ticker.c_str())) {
      Order* o = it->second;
      if (o->Valid()) {
        o->action = OrderAction::CancelOrder;
        o->status = OrderStatus::Cancelling;
        snprintf(o->order_ref, sizeof(o->order_ref), "%s", it->first.c_str());
        if (e_s) {
          o->Show(stdout);
        }
        if (e_f) {
          o->Show(order_file);
        }
        sender->Send(*o);
      } else if (o->status == OrderStatus::Modifying) {
        o->action = OrderAction::CancelOrder;
        o->status = OrderStatus::Cancelling;
      }
    }
  }
  pthread_mutex_unlock(&mod_mutex);
}

void Strategy::CancelAllHedge() {
  printf("Enter Cancel ALL Hedge\n");
  pthread_mutex_lock(&mod_mutex);
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, hedge_ticker.c_str())) {
      Order* o = it->second;
      if (o->Valid()) {
        o->action = OrderAction::CancelOrder;
        o->status = OrderStatus::Cancelling;
        snprintf(o->order_ref, sizeof(o->order_ref), "%s", it->first.c_str());
        if (e_s) {
          o->Show(stdout);
        }
        if (e_f) {
          o->Show(order_file);
        }
        sender->Send(*o);
      } else if (o->status == OrderStatus::Modifying) {
        o->action = OrderAction::CancelOrder;
        o->status = OrderStatus::Cancelling;
      }
    }
  }
  pthread_mutex_unlock(&mod_mutex);
}

void Strategy::ClearValidOrder() {
  CancelAllMain();
  CancelAllHedge();
}

void Strategy::DelOrder(std::string ref) {
  std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.find(ref);
  if (it != order_map.end()) {
    order_map.erase(it);
  } else {
    printf("Delorder Error! %s not found\n", ref.c_str());
    exit(1);
  }
}

void Strategy::UpdatePos(Order* o, ExchangeInfo info) {
  std::string contract = o->contract;
  int previous_pos = position_map[contract];
  int trade_size = (o->side == OrderSide::Buy)?o->size:-o->size;
  double trade_price = info.trade_price;
  position_map[contract] += trade_size;
  bool is_close = TradeClose(contract, trade_size);
  if (!is_close) {  // only update avgcost when open traded
    UpdateAvgCost(contract, trade_price, trade_size);
  }
  OrderSide::Enum sd;
  OrderSide::Enum reverse_sd;
  if (trade_size > 0) {
    sd = OrderSide::Buy;
    reverse_sd = OrderSide::Sell;
  } else {
    sd = OrderSide::Sell;
    reverse_sd = OrderSide::Buy;
  }

  if (contract == main_ticker) {
    int main_pos = position_map[contract];
    if (!is_close) {  // open traded
      if (main_pos*trade_size == 1) {  // pos 0->1: cancel open order, add close order, add open order
        printf("opentraded and pos=1\n");
        CancelAllMain();  // cancel open
        NewOrder(main_ticker, reverse_sd);
        NewOrder(main_ticker, sd, true);  // add open
      } else {  // pos > 1
        printf("opentraded and pos>1\n");
        if (abs(main_pos) < max_pos) {  // add close, add open
          AddCloseOrderSize(reverse_sd);
          NewOrder(main_ticker, sd, true);  // add open
        } else if (abs(main_pos) == max_pos) {  // add close only
          AddCloseOrderSize(reverse_sd);
        } else {
          SimpleHandle(240);
        }
      }
    } else {  // close traded
      if (abs(previous_pos) == max_pos) {
        NewOrder(main_ticker, reverse_sd);  // make up one open
      }
      if (main_pos == 0) {
        NewOrder(main_ticker, sd);  // add open
        return;
      }
    }
  } else if (contract == hedge_ticker) {
  } else {
    SimpleHandle(251);
  }
}

void Strategy::UpdateExchangeInfo(ExchangeInfo info) {
  printf("enter UpdateExchangeInfo\n");
  if (e_s) {
    info.Show(stdout);
  }
  if (e_f) {
    info.Show(exchange_file);
  }
  InfoType::Enum t = info.type;

  if (t == InfoType::Position) {
    if (position_ready) {  // ignore positioninfo after ready
      return;
    }
    if ((info.trade_price < 0.00001 || abs(info.trade_size) == 0) && strcmp(info.contract, "positionend") != 0) {
      return;
    }
    if (strcmp(info.contract, main_ticker.c_str()) == 0) {
      if (position_map[info.contract] + info.trade_size == 0) {
        avgcost_map[main_ticker] = 0.0;
      } else {
        avgcost_map[main_ticker] = (info.trade_price/m_contract_size*info.trade_size + avgcost_map[main_ticker]*position_map[info.contract])/(position_map[info.contract] + info.trade_size);
      }
    } else if (strcmp(info.contract, hedge_ticker.c_str()) == 0) {
      if (position_map[info.contract] + info.trade_size == 0) {
        avgcost_map[hedge_ticker] = 0.0;
      } else {
        avgcost_map[hedge_ticker] = (info.trade_price/m_contract_size*info.trade_size + avgcost_map[hedge_ticker]*position_map[info.contract])/(position_map[info.contract] + info.trade_size);
      }
    } else if (strcmp(info.contract, "positionend") == 0) {
      printf("position recv finished: %s:%d@%lf %s:%d@%lf\n", main_ticker.c_str(), position_map[main_ticker], avgcost_map[main_ticker], hedge_ticker.c_str(), position_map[hedge_ticker], avgcost_map[hedge_ticker]);
      position_ready = true;
      return;
    } else {
      printf("recv unknown contract %s\n", info.contract);
      return;
    }
    position_map[info.contract] += info.trade_size;
    return;
  }
  std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.find(info.order_ref);
  if (it == order_map.end()) {  // not main
      printf("unknown orderref!%s\n", info.order_ref);
      return;
  }
  Order* order = it->second;

  switch (t) {
    case InfoType::Acc:
    {
      if (order->status == OrderStatus::SubmitNew) {
        order->status = OrderStatus::New;
      } else {
        // TODO(nick): ignore other state?
        return;
      }
    }
      // filter the mess order of info arrived
      break;
    case InfoType::Rej:
    {
      order->status = OrderStatus::Rejected;
      DelOrder(info.order_ref);
    }
      break;
    case InfoType::Cancelled:
    {
      order->status = OrderStatus::Cancelled;
      DelOrder(info.order_ref);
      if (order->action == OrderAction::ModOrder) {
        NewOrder(order->contract, order->side, order->size);
      }
    }
      break;
    case InfoType::CancelRej:
    {
      if (order->status == OrderStatus::Filled) {
        printf("cancelrej bc filled!%s\n", order->order_ref);
        return;
      }
      order->status = OrderStatus::CancelRej;
      printf("cancel rej for order %s\n", info.order_ref);
      // TODO(nick):
      // case: cancel filled: ignore
      // case: not permitted in this time, wait to cancel
      // other reason: make up for the cancel failed
    }
      break;
    case InfoType::Filled:
    {
      order->traded_size += info.trade_size;
      if (order->size == order->traded_size) {
        order->status = OrderStatus::Filled;
        DelOrder(info.order_ref);
      } else {
        order->status = OrderStatus::Pfilled;
      }
      if (strcmp(order->contract, main_ticker.c_str()) == 0) {
        NewOrder(hedge_ticker, (order->side == OrderSide::Buy)?OrderSide::Sell : OrderSide::Buy, info.trade_size);
      } else if (strcmp(order->contract, hedge_ticker.c_str()) == 0) {
      } else {
        // TODO(nick): handle error
        SimpleHandle(322);
      }
      UpdatePos(order, info);
    }
      break;
    case InfoType::Pfilled:
      // TODO(nick): need to realize
      break;
    default:
      // TODO(nick): handle unknown info
      SimpleHandle(331);
      break;
  }
}
