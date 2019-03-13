#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#include "./strategy.h"

#define MAXINT 9999999999

Strategy::Strategy(const libconfig::Setting & param_setting, const libconfig::Setting & contract_setting, TimeController tc, std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map, std::string mode)
  : this_tc(tc),
    mode(mode),
    closed_size(0),
    build_position_time(MAXINT),
    last_valid_mid(0.0),
    stop_loss_times(0),
    max_close_try(10) {
  e_f = true;
  MarketSnapshot shot;
  try {
    cancel_limit = contract_setting["cancel_limit"];
    min_price_move = contract_setting["min_price_move"];
    open_fee_rate = contract_setting["open_fee_rate"];
    close_today_fee_rate = contract_setting["close_today_fee_rate"];
    close_fee_rate = contract_setting["close_fee_rate"];
    deposit_rate = contract_setting["deposit_rate"];
    contract_size = contract_setting["contract_size"];
    shot_map[main_ticker] = shot;
    shot_map[hedge_ticker] = shot;
    avgcost_map[main_ticker] = 0.0;
    avgcost_map[hedge_ticker] = 0.0;
    std::string unique_name = param_setting["unique_name"];
    m_strat_name = unique_name;
    std::string ticker1 = param_setting["pairs"][0];
    std::string ticker2 = param_setting["pairs"][1];
    main_ticker = ticker1;
    hedge_ticker = ticker2;
    max_pos = param_setting["max_position"];
    min_train_sample = param_setting["min_train_samples"];
    double m_r = param_setting["min_range"];
    double m_p = param_setting["min_profit"];
    min_profit = m_p * min_price_move;
    min_range = m_r * min_price_move;
    double add_margin = param_setting["add_margin"];
    increment = add_margin*min_price_move;
    double spread_threshold_int = param_setting["spread_threshold"];
    spread_threshold = spread_threshold_int*min_price_move;
    stop_loss_margin = param_setting["stop_loss_margin"];
    max_loss_times = param_setting["max_loss_times"];
    max_holding_sec = param_setting["max_holding_sec"];
    range_width = param_setting["range_width"];
  } catch(const libconfig::SettingNotFoundException &nfex) {
    printf("Setting '%s' is missing", nfex.getPath());
    exit(1);
  } catch(const libconfig::SettingTypeException &tex) {
    printf("Setting '%s' has the wrong type", tex.getPath());
    exit(1);
  } catch (const std::exception& ex) {
    printf("EXCEPTION: %s\n", ex.what());
    exit(1);
  }
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
  printf("[%s %s] %ld mid_value size is %zu\n", main_ticker.c_str(), hedge_ticker.c_str(), shot_map[hedge_ticker].time.tv_sec, map_vector.size());
  ss = StrategyStatus::Init;
  avgcost_map.clear();
  position_map.clear();
  mid_map.clear();
  map_vector.clear();
  build_position_time = MAXINT;
  stop_loss_times = 0;
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
  // printf("judge open logic side:mid = %lf, up_diff=%lf, down_diff=%lf\n", mid, up_diff, down_diff);
  // shot_map[main_ticker].Show(stdout);
  // shot_map[hedge_ticker].Show(stdout);
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
  if (cancel_map[o->contract] > cancel_limit) {
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
  /*
  int pos = position_map[main_ticker];
  if (pos != 0) {
    return;
  }
  */
  if (map_vector.size() < min_train_sample) {
    printf("[%s %s]no enough mid data! size if %zu\n", main_ticker.c_str(), hedge_ticker.c_str(), map_vector.size());
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
  round_fee_cost = (shot_map[main_ticker].bids[0] + shot_map[main_ticker].asks[0])/2 * deposit_rate * (open_fee_rate + close_today_fee_rate) *2;
  double margin = std::max(range_width * std, min_range) + round_fee_cost;
  up_diff = avg + margin;
  down_diff = avg - margin;
  stop_loss_up_line = up_diff + stop_loss_margin * margin;
  stop_loss_down_line = down_diff - stop_loss_margin * margin;
  // down_diff = std::min(avg - range_width * std, avg-min_profit);
  mean = avg;
  // spread_threshold = (margin - min_profit - round_fee_cost) / 2;
  spread_threshold = margin - min_profit;  //  - round_fee_cost;
  printf("[%s %s]cal done,mean is %lf, std is %lf, parmeters: [%lf,%lf], spread_threshold is %lf, min_profit is %lf, up_loss=%lf, down_loss=%lf fee_point=%lf\n", main_ticker.c_str(), hedge_ticker.c_str(), avg, std, down_diff, up_diff, spread_threshold, min_profit, stop_loss_up_line, stop_loss_down_line, round_fee_cost);
}

bool Strategy::HitMean() {
  double this_mid = map_vector.back();
  int pos = position_map[main_ticker];
  if ((pos > 0 && this_mid >= mean) || (pos < 0 && this_mid <= mean)) {
    printf("[%s %s] mean is %lf, this_mid is %lf, pos is %d\n", main_ticker.c_str(), hedge_ticker.c_str(), mean, this_mid, pos);
    return true;
  }
  return false;
  /*
  printf("%ld [%s %s] last_valid_mid %lf this_mid %lf\n", shot_map[main_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), last_valid_mid, this_mid);
  if ((last_valid_mid > mean && this_mid <= mean) || (last_valid_mid < mean && this_mid >= mean)) {  // last shot > mean, this <= mean or vice verse
    return true;
  }
  return false;
  */
}

void Strategy::ForceFlat() {
  printf("%ld [%s %s]this round hit stop_loss condition, pos:%d current_mid:%lf, stoplossline %lf forceflat\n", shot_map[hedge_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), position_map[main_ticker], map_vector.back(), stop_loss_down_line);
  for (int i = 0; i < max_close_try; i++) {
    if (Close(true)) {
      break;
    }
    if (i == max_close_try - 1) {
      printf("[%s %s]try max_close times, cant close this order!\n", main_ticker.c_str(), hedge_ticker.c_str());
      PrintMap(order_map);
      order_map.clear();  // it's a temp solution, TODO
    }
  }
  CalParams();
}

bool Strategy::TimeUp() {
  long int holding_sec;
  if (mode == "real") {
    holding_sec = m_tc->GetCurrentSec() - build_position_time;
  } else if (mode == "test") {
    holding_sec = shot_map[main_ticker].time.tv_sec - build_position_time;
  } else {
    printf("unknown mode %s\n", mode.c_str());
    return false;
  }
  if (holding_sec > max_holding_sec) {
    printf("[%s %s]time up %ld %ld %ld\n", main_ticker.c_str(), hedge_ticker.c_str(), holding_sec, build_position_time, holding_sec+build_position_time);
    return true;
  }
  return false;
}

bool Strategy::Close(bool force_flat) {
  int pos = position_map[main_ticker];
  if (pos == 0) {
    return true;
  }
  OrderSide::Enum side = pos > 0 ? OrderSide::Sell: OrderSide::Buy;
  printf("close using %s: pos is %d, diff is %lf\n", OrderSide::ToString(side), pos, map_vector.back());
  // printf("spread is %lf %lf min_profit is %lf\n", shot_map[main_ticker].asks[0]-shot_map[main_ticker].bids[0], shot_map[hedge_ticker].asks[0]-shot_map[hedge_ticker].bids[0], min_profit);
  if (order_map.empty()) {
    PrintMap(avgcost_map);
    Order* o;
    o = NewOrder(main_ticker, side, abs(pos), false, false, force_flat ? "force_flat" : "", mode == "test");  // close
    if (mode == "test") {
      o->wrap_time = shot_map[hedge_ticker].time;
      usleep(200000);
    }
    shot_map[main_ticker].Show(stdout);
    shot_map[hedge_ticker].Show(stdout);
    closed_size += abs(pos);
    /*
    if (mode == "test") {
      ExchangeInfo info;
      info.trade_size = o->size;
      DoOperationAfterFilled(o, info);
    }
    */
    double this_round_pnl = (pos > 0) ? (shot_map[main_ticker].bids[0] - avgcost_map[main_ticker] + avgcost_map[hedge_ticker] - shot_map[hedge_ticker].asks[0])*pos : (shot_map[main_ticker].asks[0] - avgcost_map[main_ticker] + avgcost_map[hedge_ticker] - shot_map[hedge_ticker].bids[0])*pos;
    this_round_pnl -= round_fee_cost*abs(pos);
    printf("%ld [%s %s]%sThis round close pnl: %lf, fee_cost: %lf pos is %d, holding second is %ld\n", shot_map[hedge_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), force_flat ? "[Time up] " : "", this_round_pnl, round_fee_cost, pos, shot_map[hedge_ticker].time.tv_sec - build_position_time);
    build_position_time = MAXINT;
    return true;
  } else {
    printf("block order exsited! no close\n");
    PrintMap(order_map);
    // exit(1);
    return false;
  }
}

void Strategy::StopLossLogic() {
  int pos = position_map[main_ticker];
  if (pos > 0) {  // buy position
    if (map_vector.back() < stop_loss_down_line) {  // stop condition meets
      ForceFlat();
      // printf("%ld [%s %s]this round hit stop_loss condition, pos:%d current_mid:%lf, stoplossline %lf forceflat\n", shot_map[hedge_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), pos, map_vector.back(), stop_loss_down_line);
      stop_loss_times += 1;
    }
  } else if (pos < 0) {  // sell position
    if (map_vector.back() > stop_loss_up_line) {  // stop condition meets
      ForceFlat();
      // printf("%ld [%s %s]this round hit stop_loss condition, pos:%d current_mid:%lf, stoplossline %lf forceflat\n", shot_map[hedge_ticker].time.tv_sec, main_ticker.c_str(), hedge_ticker.c_str(), pos, map_vector.back(), stop_loss_up_line);
      stop_loss_times += 1;
    }
  }
  if (stop_loss_times >= max_loss_times) {
    ss = StrategyStatus::Stopped;
    printf("stop loss times hit max!\n");
  }
}

void Strategy::CloseLogic() {
  StopLossLogic();
  int pos = position_map[main_ticker];
  if (pos == 0) {
    return;
  }

  if (TimeUp()) {
    printf("[%s %s] holding time up, start from %ld, now is %ld, close diff is %lf force to close position!\n", main_ticker.c_str(), hedge_ticker.c_str(), build_position_time, mode == "test" ? shot_map[main_ticker].time.tv_sec : m_tc->GetCurrentSec(), map_vector.back());
    // while (!Close(true));  // must close it
    ForceFlat();
    return;
  }

  if (HitMean()) {
    if (Close()) {
      CalParams();
    }
    return;
  }
}

/*
void Strategy::ForceClose() {
  int main_pos = position_map[main_ticker];
  int hedge_pos = position_map[hedge_ticker];
  Order* o1 = NewOrder(main_ticker, main_pos > 0 ? OrderSide::Sell : OrderSide::Buy, main_pos, false, false, "force close", mode == "test");
  Order* o2 = NewOrder(hedge_ticker, hedge_pos > 0 ? OrderSide::Sell : OrderSide::Buy, hedge_pos, false, false, "force close", mode == "test");
  printf("[%s %s]force close at %ld, avgcost %lf %lf, force close %lf %lf | %lf %lf, pos is %d fc pnl = %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), o2->time.tv_sec, avgcost_map[main_ticker], avgcost_map[hedge_ticker], shot_map[main_ticker].bids[0], shot_map[main_ticker].asks[0], shot_map[hedge_ticker].bids[0], shot_map[hedge_ticker].asks[0], position_map[main_ticker], );
}
*/

void Strategy::Flatting() {
  if (!IsAlign() || !Spread_Good()) {
    CloseLogic();
  }
}

/*
bool Strategy::CheckBlockOrder() {
  return !order_map.empty(); 
}
*/

void Strategy::Open(OrderSide::Enum side) {
  int pos = position_map[main_ticker];
  printf("[%s %s] open %s: pos is %d, diff is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), OrderSide::ToString(side), pos, map_vector.back());
  if (order_map.empty()) {  // no block order, can add open
    Order*o = NewOrder(main_ticker, side, 1, false, false, "", mode == "test");
    if (mode == "test") {
      o->wrap_time = shot_map[hedge_ticker].time;
      usleep(200000);
    }
    shot_map[main_ticker].Show(stdout);
    shot_map[hedge_ticker].Show(stdout);
    // TODO(nick): update up/down when true filled
    /*
    if (side == OrderSide::Buy) {
      down_diff -= increment;
    } else {
      up_diff += increment;
    }
    */
    // printf("spread is %lf %lf min_profit is %lf, next open will be %lf\n", shot_map[main_ticker].asks[0]-shot_map[main_ticker].bids[0], shot_map[hedge_ticker].asks[0]-shot_map[hedge_ticker].bids[0], min_profit, side == OrderSide::Buy ? down_diff: up_diff);
    /*
    if (mode == "test") {
      ExchangeInfo info;
      info.trade_size = o->size;
      DoOperationAfterFilled(o, info);
    }
    */
    // TODO(nick):  record open time when true filled
    if (pos == 0) {
      if (mode == "test") {
        build_position_time = shot_map[main_ticker].time.tv_sec;
      } else {
        build_position_time = m_tc->GetCurrentSec();
      }
    }
    // side == OrderSide::Buy ? current_pos++:current_pos--;
  } else {  // block order exsit, no open, possible reason: no enough margin
    printf("block order exsited! no open \n");
    PrintMap(order_map);
    // exit(1);
  }
}

bool Strategy::OpenLogic() {
  OrderSide::Enum side = OpenLogicSide();
  if (side == OrderSide::Unknown) {
    return false;
  }
  // do meet the logic
  int pos = position_map[main_ticker];
  if (abs(pos) == max_pos) {
    // hit max, still update bound
    UpdateBound(side == OrderSide::Buy ? OrderSide::Sell : OrderSide::Buy);
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
    // last_valid_mid = map_vector.back();
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
        MarketSnapshot shot = shot_map[contract];
        Order* o = it->second;
        if (o->Valid()) {
          double reasonable_price = (o->side == OrderSide::Buy ? shot.asks[0] : shot.bids[0]);
          if (fabs(o->price - reasonable_price) >= min_price_move) {
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

void Strategy::UpdateBound(OrderSide::Enum side) {
  printf("Entering UpdateBound\n");
  int pos = position_map[main_ticker];
  if (pos == 0) {  // close operation filled, no update bound
    return;
  }
  if (side == OrderSide::Sell) {
    down_diff = map_vector.back();
    down_diff -= increment;
    if (abs(pos) > 1) {
      mean -= increment/2;
      stop_loss_down_line -= increment/2;
    }
  } else {
    up_diff = map_vector.back();
    up_diff += increment;
    if (abs(pos) > 1) {
      mean += increment/2;
      stop_loss_up_line += increment/2;
    }
  }
  printf("spread is %lf %lf min_profit is %lf, next open will be %lf mean is %lf\n", shot_map[main_ticker].asks[0]-shot_map[main_ticker].bids[0], shot_map[hedge_ticker].asks[0]-shot_map[hedge_ticker].bids[0], min_profit, side == OrderSide::Sell ? down_diff: up_diff, mean);
}

void Strategy::DoOperationAfterFilled(Order* o, ExchangeInfo info) {
  if (strcmp(o->contract, main_ticker.c_str()) == 0) {
    // printf("Mid report: main_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
    // printf("hedge order for %s\n", o->order_ref);
    Order* o1 = NewOrder(hedge_ticker, (o->side == OrderSide::Buy)?OrderSide::Sell : OrderSide::Buy, info.trade_size, false, false, "", mode == "test");  // hedge operation
    if (mode == "test") {
      o->wrap_time = shot_map[hedge_ticker].time;
      // usleep(200000);
    }
    o1->Show(stdout);
  } else if (strcmp(o->contract, hedge_ticker.c_str()) == 0) {
    // printf("mid report: hedge_ticker's filled at %lf for order %s\n", info.trade_price, o->order_ref);
    UpdateBound(o->side);
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
    printf("[%s %s]spread too wide: current is %lf, and threashold is %lf\n", main_ticker.c_str(), hedge_ticker.c_str(), current_spread, spread_threshold);
    return false;
  }
  return true;
}
