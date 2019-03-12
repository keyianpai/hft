#ifndef TIMECONTROLLER_H_
#define TIMECONTROLLER_H_

#include "market_snapshot.h"
#include "order.h"
#include "sender.h"
#include "exchange_info.h"
#include "order_status.h"
#include "useful_function.h"
#include "time_status.h"
#include <tr1/unordered_map>
#include <sys/time.h>

#include <cmath>
#include <vector>
#include <iostream>
#include <string>

class TimeController {
 public:
  explicit TimeController(std::vector<std::string>sleep_time, std::vector<std::string> close_time, std::vector<std::string> force_close, std::string mode="run");
  TimeController(const TimeController & t);

  ~TimeController();

  TimeStatus::Enum CheckCurrentTime(MarketSnapshot shot);

  TimeStatus::Enum CheckTime(int check_time);

  int GetCurrentSec();
  bool IsMix(int s1, int e1, int s2, int e2);

 private:
  int last_sec;
  int timezone_diff;
  std::vector<int>sleep_start;
  std::vector<int>sleep_stop;
  std::vector<int>close_start;
  std::vector<int>close_stop;
  std::vector<int>force_close_start;
  std::vector<int>force_close_stop;
  std::string mode;

  int Translate(std::string time);
  bool Check();
  void Push(std::vector<std::string> timestr, std::vector<int>& a, std::vector<int>& b);
};

#endif  // TIMECONTROLLER_H_
