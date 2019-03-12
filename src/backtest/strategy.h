#ifndef SRC_BACKTEST_STRATEGY_H_
#define SRC_BACKTEST_STRATEGY_H_

#include <market_snapshot.h>
#include <strategy_status.h>
#include <timecontroller.h>
#include <order.h>
#include <sender.h>
#include <exchange_info.h>
#include <order_status.h>
#include <common_tools.h>
#include <base_strategy.h>
#include <libconfig.h++>
#include <tr1/unordered_map>

#include <cmath>
#include <vector>
#include <string>


class Strategy : public BaseStrategy {
 public:
  Strategy(const libconfig::Setting & param_setting, const libconfig::Setting & contract_setting, TimeController tc, std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*ticker_strat_map, std::string mode = "real");
  ~Strategy();

  void Start();
  void Stop();

  void Clear();
 private:
  void DoOperationAfterUpdateData(MarketSnapshot shot);
  void DoOperationAfterUpdatePos(Order* o, ExchangeInfo info);
  void DoOperationAfterFilled(Order* o, ExchangeInfo info);
  void DoOperationAfterCancelled(Order* o);
  void ModerateOrders(std::string contract);
  void InitTicker();
  void InitTimer();
  bool Ready();
  void Pause();
  void Resume();
  void Run();
  void Train();
  void Flatting();

  double OrderPrice(std::string contract, OrderSide::Enum side, bool control_price);

  OrderSide::Enum OpenLogicSide();
  bool OpenLogic();
  void CloseLogic();

  void Open(OrderSide::Enum side);
  bool Close(bool force_flat = false);

  bool TimeUp();

  void CalParams();
  bool HitMean();

  void ForceFlat();

  bool Spread_Good();

  bool IsAlign();

  void UpdateBound(OrderSide::Enum side);
  void StopLossLogic();

  char order_ref[MAX_ORDERREF_SIZE];
  std::string main_ticker;
  std::string hedge_ticker;
  int max_pos;
  double min_price_move;

  TimeController this_tc;
  int cancel_limit;
  std::tr1::unordered_map<std::string, double> mid_map;
  double up_diff;
  double down_diff;
  double range_width;
  double mean;
  std::vector<double> map_vector;
  int current_pos;
  double min_profit;
  unsigned int min_train_sample;
  double min_range;
  double increment;
  std::string mode;
  double spread_threshold;
  int closed_size;
  int max_holding_sec;
  long int build_position_time;
  double last_valid_mid;
  double stop_loss_up_line;
  double stop_loss_down_line;
  int max_loss_times;
  double stop_loss_times;
  double stop_loss_margin;
  double open_fee_rate;
  double close_today_fee_rate;
  double close_fee_rate;
  double deposit_rate;
  double round_fee_cost;
  int max_close_try;
};

#endif  // SRC_BACKTEST_STRATEGY_H_
