#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#include "./strategy.h"

#define MAXINT 999999999

Strategy::Strategy(const libconfig::Setting & setting, TimeController tc, std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map, std::string mode)
  : this_tc(tc),
    current_pos(0),
    mode(mode),
    closed_size(0),
    build_position_time(MAXINT),
    last_valid_mid(0.0) {
  MarketSnapshot shot;
  shot_map[main_ticker] = shot;
  shot_map[hedge_ticker] = shot;
  avgcost_map[main_ticker] = 0.0;
  avgcost_map[hedge_ticker] = 0.0;
  current_pos = 0;
  std::string unique_name = setting["unique_name"];
  m_strat_name = unique_name;
  std::string ticker1 = setting["pairs"][0];
  std::string ticker2 = setting["pairs"][1];
  main_ticker = ticker1;
  hedge_ticker = ticker2;
  min_price = setting["tick_size"];
  m_contract_size = setting["contract_size"];
  max_pos = setting["max_position"];
  min_train_sample = setting["min_train_samples"];
  cancel_threshhold = setting["cancel_threshhold"];
  double min_range = setting["min_range"];
  min_profit = min_range*min_price;
  double add_margin = setting["add_margin"];
  double spread_threshold_int = setting["spread_threshold"];
  spread_threshold = spread_threshold_int*min_price;
  max_holding_sec = setting["max_holding_sec"];
  increment = add_margin*min_price;
  (*ticker_strat_map)[main_ticker].push_back(this);
  (*ticker_strat_map)[hedge_ticker].push_back(this);
  (*ticker_strat_map)["positionend"].push_back(this);
  if (mode == "test") {
    position_ready = true;
  }
}

Strategy::~Strategy() {
  delete sender;
}

void Strategy::Clear() {
  current_pos = 0;
  mid_map.clear();
  map_vector.clear();
}

void Strategy::Stop() {
  CancelAll(main_ticker);
  ss = StrategyStatus::Stopped;
}

bool Strategy::IsAlign() {
  if (shot_map[main_ticker].time.tv_sec == shot_map[hedge_ticker].time.tv_sec && abs(shot_map[main_ticker].time.tv_usec-shot_map[hedge_ticker].time.tv_usec) < 10000) {
    return true;
  }
  return false;
}

OrderSide::Enum Strategy::OpenLogicSide() {
  double mid = (shot_map[main_ticker].bids[0]+shot_map[main_ticker].asks[0])/2
             - (shot_map[hedge_ticker].bids[0]+shot_map[hedge_ticker].asks[0])/2;
  if (mid > up_diff) {
    if (mode == "real") {
      printf("[%s %s]sell condition hit, as diff id %f\n",  main_ticker.c_str(), hedge_ticker.c_str(), mid);
    }
    return OrderSide::Sell;
  } else if (mid < down_diff) {
    if (mode == "real") {
      printf("[%s %s]buy condition hit, as diff id %f\n", main_ticker.c_str(), hedge_ticker.c_str(), mid);
    }
    return OrderSide::Buy;
  } else {
    return OrderSide::Unknown;
  }
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

void Strategy::CalParams() {
  if (map_vector.size() < min_train_sample) {
    printf("no enough mid data!\n");  // should not happen
    exit(1);
  }
  double avg = 0.0;
  double std = 0.0;
  int cal_head = map_vector.size() - min_train_sample;
  for (unsigned int i = cal_head; i < map_vector.size(); i++) {
    avg += map_vector[i];
  }
  avg /= min_train_sample;
  for (unsigned int i = cal_head; i < map_vector.size(); i++) {
    std += (map_vector[i]-avg) * (map_vector[i]-avg);
  }
  std /= min_train_sample;
  std = sqrt(std);
  up_diff = std::max(avg + 2 * std, avg+min_profit);
  down_diff = std::min(avg - 2 * std, avg-min_profit);
  mean = avg;
  spread_threshold = 2*std-min_profit;
  printf("[%s %s]cal done,mean is %lf, std is %lf, parmeters: [%lf,%lf], spread_threshold is %lf, min_profit is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), avg, std, down_diff, up_diff, spread_threshold, min_profit);
}

bool Strategy::HitMean() {
  double this_mid = map_vector.back();
  printf("%ld [%s %s] last_valid_mid %lf this_mid %lf\n", shot_map[main_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), last_valid_mid, this_mid);
  if ((last_valid_mid > mean && this_mid <= mean) || (last_valid_mid < mean && this_mid >= mean)) {  // last shot > mean, this <= mean or vice verse
    return true;
  }
  return false;
}

bool Strategy::TimeUp() {
  int holding_sec;
  if (mode == "real") {
    holding_sec = m_tc->GetCurrentSec() - build_position_time;
  } else if (mode == "test") {
    holding_sec = shot_map[main_ticker].time.tv_sec - build_position_time;
  } else {
    printf("unknown mode %s\n", mode.c_str());
    return false;
  }
  if (holding_sec > max_holding_sec) {
    return true;
  }
  return false;
}

void Strategy::Close(bool force_flat) {
  PrintMap(avgcost_map);
  Order* o;
  if (force_flat) {
    o = NewOrder(main_ticker, current_pos > 0 ? OrderSide::Sell: OrderSide::Buy, abs(current_pos), false, false, "force_flat");  // close
  } else {
    o = NewOrder(main_ticker, current_pos > 0 ? OrderSide::Sell: OrderSide::Buy, abs(current_pos), false, false);  // close
  }
  if (mode == "test") {
    usleep(200000);
  }
  printf("close using %s: pos is %d, diff is %lf\n", OrderSide::ToString(o->side), current_pos, map_vector.back());
  shot_map[main_ticker].Show(stdout);
  shot_map[hedge_ticker].Show(stdout);
  printf("spread is %lf %lf min_profit is %lf\n", shot_map[main_ticker].asks[0]-shot_map[main_ticker].bids[0], shot_map[hedge_ticker].asks[0]-shot_map[hedge_ticker].bids[0], min_profit);
  closed_size += abs(current_pos);
  /*
  if (mode == "test") {
    ExchangeInfo info;
    info.trade_size = o->size;
    DoOperationAfterFilled(o, info);
  }
  */
  double this_round_pnl = (current_pos > 0) ? (shot_map[main_ticker].bids[0] - avgcost_map[main_ticker] + avgcost_map[hedge_ticker] - shot_map[hedge_ticker].asks[0])*current_pos : (shot_map[main_ticker].asks[0] - avgcost_map[main_ticker] + avgcost_map[hedge_ticker] - shot_map[hedge_ticker].bids[0])*current_pos;
  printf("[%s %s]%sThis round close pnl: %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), force_flat ? "[Time up] " : "", this_round_pnl);
  current_pos = 0;
  build_position_time = MAXINT;
}

void Strategy::CloseLogic() {
  if (current_pos == 0) {
    return;
  }

  if (TimeUp()) {
    printf("[%s %s] holding time up, start from %d, now is %ld, close diff is %lf force to close position!\n", main_ticker.c_str(), hedge_ticker.c_str(), build_position_time, mode == "test" ? shot_map[main_ticker].time.tv_sec : m_tc->GetCurrentSec(), map_vector.back());
    Close(true);
    CalParams();
    return;
  }

  if (HitMean()) {
    Close();
    CalParams();
    return;
  }
}

void Strategy::Flatting() {
  if (!IsAlign() || !Spread_Good()) {
    CloseLogic();
  }
}

void Strategy::Open(OrderSide::Enum side) {
  Order* o = NewOrder(main_ticker, side, 1, false, false);
  if (mode == "test") {
    usleep(200000);
  }
  printf("[%s %s] open %s: pos is %d, diff is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), OrderSide::ToString(o->side), current_pos, map_vector.back());
  shot_map[main_ticker].Show(stdout);
  shot_map[hedge_ticker].Show(stdout);
  if (side == OrderSide::Buy) {
    down_diff = map_vector.back();
    down_diff -= increment;
  } else {
    down_diff = map_vector.back();
    up_diff += increment;
  }
  printf("spread is %lf %lf min_profit is %lf, next open will be %lf\n", shot_map[main_ticker].asks[0]-shot_map[main_ticker].bids[0], shot_map[hedge_ticker].asks[0]-shot_map[hedge_ticker].bids[0], min_profit, side == OrderSide::Buy ? down_diff: up_diff);
  /*
  if (mode == "test") {
    ExchangeInfo info;
    info.trade_size = o->size;
    DoOperationAfterFilled(o, info);
  }
  */
  if (current_pos == 0) {
    if (mode == "test") {
      build_position_time = shot_map[main_ticker].time.tv_sec;
    } else {
      build_position_time = m_tc->GetCurrentSec();
    }
  }
  side == OrderSide::Buy ? current_pos++:current_pos--;
}

bool Strategy::OpenLogic() {
  if (abs(current_pos) == max_pos) {
    return false;
  }
  OrderSide::Enum side = OpenLogicSide();
  if (side == OrderSide::Unknown) {
    return false;
  }
  Open(side);
  return true;
}

void Strategy::Run() {
  if (IsAlign() && Spread_Good()) {
    if (!OpenLogic()) {
      CloseLogic();
    }
    last_valid_mid = map_vector.back();
  } else {
    // printf("");
  }
}

void Strategy::InitTicker() {
  ticker_map[main_ticker] = true;
  ticker_map[hedge_ticker] = true;
  ticker_map["positionend"] = true;
}

void Strategy::InitTimer() {
  m_tc = &this_tc;
}

void Strategy::DoOperationAfterUpdateData(MarketSnapshot shot) {
  // MarketSnapshot shot = last_shot;
  mid_map[shot.ticker] = (shot.bids[0]+shot.asks[0]) / 2;
  if (IsAlign()) {
    printf("%ld [%s, %s]mid_diff is %lf\n", shot.time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), mid_map[main_ticker]-mid_map[hedge_ticker]);
    double mid = (shot_map[main_ticker].bids[0]+shot_map[main_ticker].asks[0])/2
               - (shot_map[hedge_ticker].bids[0]+shot_map[hedge_ticker].asks[0])/2;
    map_vector.push_back(mid);
  }
}

void Strategy::Train() {
}

void Strategy::Pause() {
}

void Strategy::Resume() {
  Run();
}

bool Strategy::Ready() {
  if (position_ready && shot_map[main_ticker].IsGood() && shot_map[hedge_ticker].IsGood() && mid_map[main_ticker] > 10 && mid_map[hedge_ticker] > 10 && map_vector.size() >= min_train_sample) {
    if (map_vector.size() == min_train_sample) {
      // first cal params
      CalParams();
    }
    return true;
  }
  if (!position_ready) {
    printf("waiting position query finish!\n");
  }
  return false;
}

void Strategy::ModerateOrders(std::string contract) {
  if (mode == "real") {
    for (std::tr1::unordered_map<std::string, Order*>::iterator it = order_map.begin(); it != order_map.end(); it++) {
      if (!strcmp(it->second->contract, hedge_ticker.c_str()) || !strcmp(it->second->contract, main_ticker.c_str())) {
        MarketSnapshot shot = shot_map[it->second->contract];
        Order* o = it->second;
        if (o->Valid()) {
          double reasonable_price = (o->side == OrderSide::Buy ? shot.asks[0] : shot.bids[0]);
          if (fabs(o->price - reasonable_price) >= min_price) {
            ModOrder(o);
            printf("Slip point report:modify %s order %s: %lf->%lf\n", OrderSide::ToString(o->side), o->order_ref, o->price, reasonable_price);
          }
        }
      }
    }
  }
}

void Strategy::Start() {
  Run();
}

void Strategy::DoOperationAfterUpdatePos(Order* o, ExchangeInfo info) {
}

void Strategy::DoOperationAfterFilled(Order* o, ExchangeInfo info) {
  if (strcmp(o->contract, main_ticker.c_str()) == 0) {
    // printf("Mid report: main_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
    // printf("hedge order for %s\n", o->order_ref);
    NewOrder(hedge_ticker, (o->side == OrderSide::Buy)?OrderSide::Sell : OrderSide::Buy, info.trade_size, false, false);  // hedge operation
    if (mode == "test") {
      usleep(200000);
    }
  } else if (strcmp(o->contract, hedge_ticker.c_str()) == 0) {
    // printf("mid report: hedge_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
  } else {
    SimpleHandle(322);
  }
}

bool Strategy::Spread_Good() {
  /*
  if (shot_map[main_ticker].asks[0] - shot_map[main_ticker].bids[0] <= spread_threshold) {
     if (shot_map[hedge_ticker].asks[0] - shot_map[hedge_ticker].bids[0] <= spread_threshold) {
       return true;
     } else {
       printf("[%s %s]hedge spread too wide!%lf, %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[hedge_ticker].asks[0], shot_map[hedge_ticker].bids[0]);
       return false;
     }
  } else {
    printf("[%s %s]main spread too wide!%lf, %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[main_ticker].asks[0], shot_map[main_ticker].bids[0]);
    return false;
  }
  */
  double current_spread = shot_map[main_ticker].asks[0] - shot_map[main_ticker].bids[0] + shot_map[hedge_ticker].asks[0] - shot_map[hedge_ticker].bids[0];
  if (current_spread > spread_threshold) {
    printf("spread too wide: current is %lf, and threashold is %lf\n", current_spread, spread_threshold);
    return false;
  }
  return true;
}
