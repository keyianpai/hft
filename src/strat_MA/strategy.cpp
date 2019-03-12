#include <iostream>
#include <string>
#include <vector>

#include "strat_MA/strategy.h"

Strategy::Strategy(std::vector<std::string> strat_contracts, std::vector<std::string> topic_v, TimeController tc, std::string strat_name)
  : m_topic_v(topic_v),
    m_tc(tc),
    m_strat_name(strat_name),
    ref_num(0) {
  if (e_f) {
    order_file = fopen("order.txt", "w");
    exchange_file = fopen("exchange.txt", "w");
  }
  pthread_mutex_init(&order_ref_mutex, NULL);
  pthread_mutex_init(&add_size_mutex, NULL);
  pthread_mutex_init(&mod_mutex, NULL);
  pthread_mutex_init(&order_map_mutex, NULL);
  sender = new Sender("order_sub");
  for (unsigned int i = 0; i < strat_contracts.size(); i++) {
    IsStratContract[strat_contracts[i]] = true;
  }
  // RequestQryPos();
}

Strategy::~Strategy() {
  delete sender;
  if (e_f) {
    fclose(order_file);
    fclose(exchange_file);
  }
}

double Strategy::OrderPrice(std::string ticker, OrderSide::Enum side, PriceMode pmode, bool control_price) {
  switch (pmode) {
    case BestPos:
     if (control_price) {
       return (side == OrderSide::Buy)?shot_map[ticker].bids[0]-price_control:shot_map[ticker].asks[0]+price_control;
     } else {
       return (side == OrderSide::Buy)?shot_map[ticker].bids[0]:shot_map[ticker].asks[0];
     }
     break;
    case MarketPrice:
     return (side == OrderSide::Buy)?shot_map[ticker].asks[0]:shot_map[ticker].bids[0];
     break;
    case NoLossBest:
     return -1;
     break;
    case NoLossMarket:
     return -1;
     break;
    default:
     printf("orderprice mode undefined!\n");
     exit(1);
     return -1;
     break;
  }
  return -1;
}

void Strategy::SimpleHandle(int line) {
  printf("unexpected error in line %d\n", line);
}

std::string Strategy::GenOrderRef() {
  char orderref[32];
  snprintf(orderref, sizeof(orderref), "%s%d", m_strat_name.c_str(), ref_num++);
  return orderref;
}

void Strategy::NewOrder(std::string contract, OrderSide::Enum side, PriceMode pmode, int size, bool control_price) {
    if (size == 0) {
      return;
    }
    pthread_mutex_lock(&order_ref_mutex);
    Order* order = new Order();
    snprintf(order->contract, sizeof(order->contract), "%s", contract.c_str());
    order->price = OrderPrice(contract, side, pmode, control_price);
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
    pthread_mutex_lock(&order_map_mutex);
    order_map[contract][order->order_ref] = order;
    pthread_mutex_unlock(&order_map_mutex);
}


void Strategy::ModerateOrders(std::string contract, ModMode mmode) {
  tr1::unordered_map<std::string, tr1::unordered_map<std::string, Order*> >::iterator it = order_map.find(contract);
  if (it == order_map.end()) {
    return;
  }
  tr1::unordered_map<std::string, Order*> temp_map = it->second;
  switch (mmode) {
    case TradeOrCancel:
     ClearAllByContract(contract);
     break;
    default:
     printf("unkonw mod mode. exit!\n");
     exit(1);
  }
}

void Strategy::Run(std::string ticker) {
  if (m_topic_v.size() < 2) {
    printf("topic size less than 2, exit!\n");
    exit(1);
  }
  std::string short_topic = m_topic_v.front();
  std::string long_topic = m_topic_v.back();  // TODO(nick): ensure size > 2
  int long_size = pricer_map[ticker][long_topic].size();
  int short_size = pricer_map[ticker][short_topic].size();
  if (long_size < 2 || short_size < 2) {
    printf("ema size < 2\n");
    return;
  }
  double longdata_pre = pricer_map[ticker][long_topic][long_size-2].data;
  double shortdata_pre = pricer_map[ticker][short_topic][short_size-2].data;
  double longdata = pricer_map[ticker][long_topic].back().data;
  double shortdata = pricer_map[ticker][short_topic].back().data;
  printf("%s longdata: pre=%lf current=%lf, shortdata: pre=%lf current=%lf\n", ticker.c_str(), longdata_pre, longdata, shortdata_pre, shortdata);
  if (longdata > shortdata && longdata_pre < shortdata_pre) {  // bear market, sell
    printf("death cross for %s, sell!\n", ticker.c_str());
    NewOrder(ticker, OrderSide::Sell, MarketPrice);
  }
  if (longdata < shortdata && longdata_pre > shortdata_pre) {  // bull market, buy
    printf("golden cross for %s, buy!\n", ticker.c_str());
    NewOrder(ticker, OrderSide::Buy, MarketPrice);
  }
}

void Strategy::UpdateData(PricerData pd) {
  pricer_map[pd.ticker][pd.topic].push_back(pd);
  shot_map[pd.ticker] = pd.shot;
  if (!IsStratContract[pd.ticker]) {
    return;
  }
  ModerateOrders(pd.ticker, TradeOrCancel);
  if (DataReady(pd.ticker, m_topic_v)) {
    Run(pd.ticker);
  }
}

bool Strategy::DataReady(std::string ticker, std::vector<std::string>topic_v) {
  if (topic_v.empty()) {
    return false;
  }
  tr1::unordered_map<std::string, tr1::unordered_map<std::string, std::vector<PricerData> > >::iterator p_i = pricer_map.find(ticker);
  if (p_i == pricer_map.end()) {
    printf("contract %s not found in pricer_map\n", ticker.c_str());
    return false;
  }
  tr1::unordered_map<std::string, std::vector<PricerData> > topic_map = p_i->second;
  tr1::unordered_map<std::string, std::vector<PricerData> >::iterator it = topic_map.find(topic_v.front());
  if (it == topic_map.end()) {
    printf("topic %s not found in topic_map\n", topic_v.front().c_str());
    return false;
  }
  if (it->second.empty()) {
    return false;
  }
  int time_sec = it->second.back().time_sec;  // first topic's sign
  for (unsigned int i = 1; i < topic_v.size(); i++) {
    if (topic_map[topic_v[i]].empty()) {
      return false;
    }
    if (topic_map[topic_v[i]].back().time_sec != time_sec) {
      return false;
    }
  }
  return true;
}

void Strategy::ClearAllByContract(std::string contract) {
  printf("Enter Cancel ALL for %s\n", contract.c_str());
  tr1::unordered_map<std::string, std::tr1::unordered_map<std::string, Order*> >::iterator itr = order_map.find(contract);
  if (itr == order_map.end()) {
    return;
  }
  std::tr1::unordered_map<std::string, Order*> temp_map = itr->second;
  pthread_mutex_lock(&mod_mutex);
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = temp_map.begin(); it != temp_map.end(); it++) {
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
  pthread_mutex_unlock(&mod_mutex);
}

void ClearAll() {
}

void Strategy::DelOrder(std::string contract, std::string ref) {
  pthread_mutex_lock(&order_map_mutex);
  std::tr1::unordered_map<std::string, std::tr1::unordered_map<std::string, Order*> >::iterator it = order_map.find(contract);
  if (it == order_map.end()) {
    printf("delete error:tickererror not found %s order %s\n", contract.c_str(), ref.c_str());
    return;
  }
  std::tr1::unordered_map<std::string, Order*> &temp_map = it->second;
  std::tr1::unordered_map<std::string, Order*>::iterator itr = temp_map.find(ref);
  if (itr == temp_map.end()) {
    printf("delete error:orderreferror not found %s order %s\n", contract.c_str(), ref.c_str());
    return;
  }
  temp_map.erase(itr);
  pthread_mutex_unlock(&order_map_mutex);
}

void Strategy::UpdatePos(Order* o, ExchangeInfo info) {
  std::string contract = o->contract;
  int trade_size = (o->side == OrderSide::Buy)?o->size:-o->size;
  double trade_price = info.trade_price;
  position_map[contract] += trade_size;
  bool is_close = TradeClose(contract, trade_size);
  if (!is_close) {  // only update avgcost when open traded
    UpdateAvgCost(contract, trade_price, trade_size);
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
    if (strcmp(info.contract, main_shot.ticker) == 0) {
      if (position_map[info.contract] + info.trade_size == 0) {
        avgcost_main = 0.0;
      } else {
        avgcost_main = (info.trade_price/m_contract_size*info.trade_size + avgcost_main*position_map[info.contract])/(position_map[info.contract] + info.trade_size);
      }
    } else if (strcmp(info.contract, hedge_shot.ticker) == 0) {
      if (position_map[info.contract] + info.trade_size == 0) {
        avgcost_hedge = 0.0;
      } else {
        avgcost_hedge = (info.trade_price/m_contract_size*info.trade_size + avgcost_hedge*position_map[info.contract])/(position_map[info.contract] + info.trade_size);
      }
    } else if (strcmp(info.contract, "positionend") == 0) {
      printf("position recv finished: %s:%d@%lf %s:%d@%lf\n", main_shot.ticker, position_map[main_shot.ticker], avgcost_main, hedge_shot.ticker, position_map[hedge_shot.ticker], avgcost_hedge);
      position_ready = true;
      return;
    } else {
      printf("recv unknown contract %s\n", info.contract);
      return;
    }
    position_map[info.contract] += info.trade_size;
    return;
  }

  std::tr1::unordered_map<std::string, std::tr1::unordered_map<std::string, Order*> >::iterator it = order_map.find(info.contract);
  if (it == order_map.end()) {
    printf("unknown ticker %s for ordermap! %s\n", info.contract, info.order_ref);
    return;
  }
  std::tr1::unordered_map<std::string, Order*>::iterator itr = it->second.find(info.order_ref);
  if (itr == it->second.end()) {
    printf("unknown ref %s for %s in ordermap\n", info.order_ref, info.contract);
    return;
  }
  Order* order = itr->second;

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
      DelOrder(order->contract, info.order_ref);
    }
      break;
    case InfoType::Cancelled:
    {
      order->status = OrderStatus::Cancelled;
      DelOrder(order->contract, info.order_ref);
      if (order->action == OrderAction::ModOrder) {
        NewOrder(order->contract, order->side, MarketPrice, order->size);
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
        DelOrder(order->contract, info.order_ref);
      } else {
        order->status = OrderStatus::Pfilled;
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
