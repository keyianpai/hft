#include <iostream>
#include <string>
#include <algorithm>

#include "simplearb/strategy.h"

Strategy::Strategy(std::string main_ticker, std::string hedge_ticker, int maxpos, double tick_size, TimeController tc, int contract_size, std::string strat_name, bool enable_stdout, bool enable_file)
  : main_ticker(main_ticker),
    hedge_ticker(hedge_ticker),
    max_pos(maxpos),
    poscapital(0.0),
    min_price(tick_size),
    price_control(10.0*min_price),
    edurance(0*min_price),
    m_tc(tc),
    cancel_threshhold(3400000),
    is_stopped(false),
    count(0),
    up_diff(25),
    down_diff(-64),
    mean(100),
    pos(0),
    min_profit(3*tick_size),
    min_train_sample(600) {
  e_s = true;
  e_f = true;
  allow_open = true;
  sender = new Sender("order_sub", "connect");
  std::string orderfile_name = strat_name + "_order.txt";
  std::string exchangefile_name = strat_name + "_exchange.txt";
  if (e_f) {
    order_file = fopen(orderfile_name.c_str(), "w");
    exchange_file = fopen(exchangefile_name.c_str(), "w");
  }
  pthread_mutex_init(&add_size_mutex, NULL);
  m_contract_size = contract_size;
  m_strat_name = strat_name;
  MarketSnapshot shot;
  shot_map[main_ticker] = shot;
  shot_map[hedge_ticker] = shot;
  avgcost_map[main_ticker] = 0.0;
  avgcost_map[hedge_ticker] = 0.0;
  ss = StrategyStatus::Init;
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

bool Strategy::MidSell() {
  if (mid_map[main_ticker] - mid_map[hedge_ticker] < down_diff) {
    printf("midsell hit, as diff id %f\n", mid_map[main_ticker] - mid_map[hedge_ticker]);
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
    printf("midbuy hit, as diff id %f\n", mid_map[main_ticker] - mid_map[hedge_ticker]);
    return false;
  }
  return true;
}

bool Strategy::PriceChange(double current_price, double reasonable_price, OrderSide::Enum side, double edurance) {
  return false;
}

void Strategy::DoOperationAfterCancelled(Order* o) {
  printf("contract %s cancel num %d!\n", o->contract, cancel_map[o->contract]);
  if (cancel_map[o->contract] > cancel_threshhold) {
    printf("contract %s hit cancel limit!\n", o->contract);
    Stop();
  }
}

double Strategy::OrderPrice(std::string contract, OrderSide::Enum side, bool control_price) {
  if (contract == hedge_ticker) {
    return (side == OrderSide::Buy)?shot_map[hedge_ticker].asks[0]:shot_map[hedge_ticker].bids[0];
  } else if (contract == main_ticker) {
    return (side == OrderSide::Buy)?shot_map[main_ticker].asks[0]:shot_map[main_ticker].bids[0];
  } else {
    printf("error contract %s\n", contract.c_str());
    return -1.0;
  }
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
    up_diff = std::max(avg + 2 * std, avg+min_profit);
    down_diff = std::min(avg - 2 * std, avg-min_profit);
    mean = avg;
    printf("[%s %s]cal done,mean is %lf, std is %lf, parmeters: [%lf,%lf]\n", main_ticker.c_str(), hedge_ticker.c_str(), avg, std, down_diff, up_diff);
    return true;
  } else if (map_vector.size() > min_train_sample) {
    return true;
  } else {
    printf("[%s %s]calculating the parmeters %zu\n", main_ticker.c_str(), hedge_ticker.c_str(), map_vector.size());
    return false;
  }
}

bool Strategy::HitMean() {
  double last_mid = map_vector[map_vector.size()-2];
  double this_mid = map_vector.back();
  if ((last_mid > mean && this_mid <= mean) || (last_mid < mean && this_mid >= mean)) {  // last shot > mean, this <= mean or vice verse
    return true;
  }
  return false;
}

void Strategy::Run() {
  if (!IsAlign()) {
    return;
  }
  if (allow_open) {
    if (!MidBuy() && abs(pos) < max_pos) {  // open sell
      printf("open sell: pos is %d, diff is %lf\n", pos, map_vector.back());
      shot_map[main_ticker].Show(stdout);
      shot_map[hedge_ticker].Show(stdout);
      NewOrder(main_ticker, OrderSide::Sell, 1, false, false);
      pos--;
      return;
    }
    if (!MidSell() && abs(pos) < max_pos) {  // open buy
      printf("open buy: pos is %d, diff is %lf\n", pos, map_vector.back());
      shot_map[main_ticker].Show(stdout);
      shot_map[hedge_ticker].Show(stdout);
      NewOrder(main_ticker, OrderSide::Buy, 1, false, false);
      pos++;
      return;
    }
  }
  if (HitMean()) {
    if (pos > 0) {  // close buy
      printf("close using sell: pos is %d, diff is %lf\n", pos, map_vector.back());
      shot_map[main_ticker].Show(stdout);
      shot_map[hedge_ticker].Show(stdout);
      NewOrder(main_ticker, OrderSide::Sell, abs(pos), false, false);  // close
      pos = 0;
    } else if (pos < 0) {
      printf("close using buy: pos is %d, diff is %lf\n", pos, map_vector.back());
      shot_map[main_ticker].Show(stdout);
      shot_map[hedge_ticker].Show(stdout);
      NewOrder(main_ticker, OrderSide::Buy, abs(pos), false, false);  // close
      pos = 0;
    } else {
    }
    return;
  }
}

void Strategy::Train() {
  printf("training [%s %s], status is %s\n", main_ticker.c_str(), hedge_ticker.c_str(), StrategyStatus::ToString(ss));
}

void Strategy::Pause() {
}

void Strategy::Resume() {
  Run();
}

void Strategy::CheckStatus() {
  switch (ss) {
    case StrategyStatus::Init:
      if (m_tc.TimeValid()) {
        if (Ready()) {
          ss = StrategyStatus::Running;
          Start();
          return;
        } else {
          ss = StrategyStatus::Training;
          Train();
          return;
        }
      }
      break;

    case StrategyStatus::Running:
      if (!m_tc.TimeValid()) {
        if (m_tc.TimeClose()) {
          allow_open = false;
          ss = StrategyStatus::Flatting;
          return;
        } else {
          Pause();
          ss = StrategyStatus::Pause;
          return;
        }
      } else {
        Run();
        return;
      }
      break;

    case StrategyStatus::Training:
      Train();
      if (Ready()) {
        ss = StrategyStatus::Running;
        Start();
      }
      return;
      break;

    case StrategyStatus::Pause:
      if (m_tc.TimeValid()) {
        ss = StrategyStatus::Running;
        Resume();
        return;
      }
      break;

    case StrategyStatus::Stopped:
      return;
      break;

    case StrategyStatus::Flatting:
      Run();
      return;
      break;

    case StrategyStatus::ForceFlat:
      return;
      break;
  }
  return;
}

bool Strategy::Ready() {
  if (position_ready && shot_map[main_ticker].IsGood() && shot_map[hedge_ticker].IsGood() && mid_map[main_ticker] > 10 && mid_map[hedge_ticker] > 10 && IsParamOK()) {
    return true;
  }
  if (!position_ready) {
    printf("waiting position query finish!\n");
  }
  return false;
}

void Strategy::DoOperationAfterUpdateData(MarketSnapshot shot) {
  std::string shot_ticker = shot.ticker;
  if (shot_ticker != main_ticker && shot_ticker != hedge_ticker) {
    return;
  }
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
  CheckStatus();
  ModerateOrders(shot_ticker, 10);
}

void Strategy::ModerateOrders(std::string contract, double edurance) {
  for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
    if (!strcmp(it->second->contract, hedge_ticker.c_str())) {
      MarketSnapshot hedge_shot = shot_map[hedge_ticker];
      Order* o = it->second;
      if (o->Valid()) {
        int hedge_pos = position_map[hedge_ticker];
        if (o->side == OrderSide::Buy && fabs(o->price - hedge_shot.asks[0]) > 0.01) {
          if (hedge_pos < 0) {  // it's a close order, if need to modify, it will be a slip of price
            fprintf(order_file, "Slip point report:modify buy order %s: %lf->%lf\n", o->order_ref, o->price, hedge_shot.asks[0]);
          }
          ModOrder(o);
        } else if (o->side == OrderSide::Sell && fabs(o->price - hedge_shot.bids[0]) > 0.01) {
          if (hedge_pos > 0) {
            fprintf(order_file, "Slip point report:modify sell order %s: %lf->%lf\n", o->order_ref, o->price, hedge_shot.bids[0]);
          }
          ModOrder(o);
        } else {
          // TODO(nick): handle error
        }
      }
    }
    if (!strcmp(it->second->contract, main_ticker.c_str())) {
      MarketSnapshot main_shot = shot_map[main_ticker];
      Order* o = it->second;
      if (o->Valid()) {
        int main_pos = position_map[main_ticker];
        if (o->side == OrderSide::Buy && fabs(o->price - main_shot.asks[0]) > 0.01) {
          if (main_pos < 0) {  // it's a close order, if need to modify, it will be a slip of price
            fprintf(order_file, "main Slip point report:modify buy order %s: %lf->%lf\n", o->order_ref, o->price, main_shot.asks[0]);
          }
          ModOrder(o);
        } else if (o->side == OrderSide::Sell && fabs(o->price - main_shot.bids[0]) > 0.01) {
          if (main_pos > 0) {
            fprintf(order_file, "main Slip point report:modify sell order %s: %lf->%lf\n", o->order_ref, o->price, main_shot.bids[0]);
          }
          ModOrder(o);
        } else {
          // TODO(nick): handle error
        }
      }
    }
  }
}

/*
bool Strategy::IsReady() {
  if (shot_map[main_ticker].is_initialized && shot_map[hedge_ticker].is_initialized && position_ready && shot_map[main_ticker].IsGood() && shot_map[hedge_ticker].IsGood() && mid_map[main_ticker] > 10 && mid_map[hedge_ticker] > 10) {
    return true;
  }
  if (!position_ready) {
    printf("waiting position query finish!\n");
  }
  return false;
}
*/

void Strategy::Start() {
  Run();
}

void Strategy::DoOperationAfterUpdatePos(Order* o, ExchangeInfo info) {
}

void Strategy::DoOperationAfterFilled(Order* o, ExchangeInfo info) {
  if (strcmp(o->contract, main_ticker.c_str()) == 0) {
    printf("Mid report: main_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
    fprintf(order_file, "hedge order for %s\n", o->order_ref);
    NewOrder(hedge_ticker, (o->side == OrderSide::Buy)?OrderSide::Sell : OrderSide::Buy, info.trade_size, false, false);  // hedge operation
  } else if (strcmp(o->contract, hedge_ticker.c_str()) == 0) {
    printf("mid report: hedge_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
  } else {
    SimpleHandle(322);
  }
}
