#ifndef SRC_STRAT_TIMECONTROLLER_H_
#define SRC_STRAT_TIMECONTROLLER_H_

#include <market_snapshot.h>
#include <order.h>
#include <sender.h>
#include <exchange_info.h>
#include <order_status.h>
#include <useful_function.h>
#include <tr1/unordered_map>
#include <sys/time.h>

#include <cmath>
#include <vector>
#include <iostream>
#include <string>

class TimeController {
 public:
  explicit TimeController(std::vector<std::string>sleep_time, std::string start_time, std::string close_time)
    : timezone_diff(8*3600),
      closepos_time(Translate(close_time)),
      m_start_time(Translate(start_time)) {
    for (size_t i = 0; i < sleep_time.size(); i++) {
      std::vector<std::string> tv = SplitStr(sleep_time[i], "-");
      if (tv.size() != 2) {
        printf("split error!size is %zd, it's %s\n", tv.size(), sleep_time[i].c_str());
        exit(1);
      }
      if (Translate(tv[0]) >= Translate(tv[1])) {
        printf("time error, stoptime < starttime! it's %s\n", sleep_time[i].c_str());
        exit(1);
      }
      sleep_start.push_back(Translate(tv[0]));
      sleep_stop.push_back(Translate(tv[1]));
    }
    printf("closepos time is %d\n", closepos_time);
    for (size_t i = 0; i < sleep_start.size(); i++) {
      printf("sleep time: %d - %d, %d:%d - %d:%d\n", sleep_start[i], sleep_stop[i], sleep_start[i]/3600, sleep_start[i]%3600/60, sleep_stop[i]/3600, sleep_stop[i]%3600/60);
    }
  }

  ~TimeController() {
  }

  bool TimeValid(int time) {
    int check_time = time % (24*3600);
    for (size_t i = 0; i < sleep_start.size(); i++) {
      if (sleep_start[i] <= check_time && check_time <= sleep_stop[i]) {
        return false;
      }
    }
    if (m_start_time > closepos_time) {  // start at night, close tomw
      if (closepos_time <= check_time && check_time < m_start_time) {
        return false;
      }
    } else {
      if (check_time >= closepos_time) {
        return false;
      }
    }
    return true;
  }

  bool TimeValid() {
    timeval current_time;
    gettimeofday(&current_time, NULL);
    return TimeValid(current_time.tv_sec + timezone_diff);
  }

  bool TimeClose() {
    timeval current_time;
    gettimeofday(&current_time, NULL);
    if ((current_time.tv_sec + timezone_diff)%(24*3600) >= closepos_time) {
      return true;
    }
    return false;
  }

 private:
  int timezone_diff;
  int closepos_time;
  int m_start_time;
  std::vector<int>sleep_start;
  std::vector<int>sleep_stop;
  int Translate(std::string time) {
    std::vector<std::string> content = SplitStr(time, ":");
    if (content.size() != 3) {
      printf("slplit time error, size is %zd, it's %s\n", content.size(), time.c_str());
      exit(1);
    }
    int hour = atoi(content[0].c_str());
    int min = atoi(content[1].c_str());
    int sec = atoi(content[2].c_str());
    return hour * 3600 + min * 60 + sec;
  }
};

#endif  // SRC_STRAT_TIMECONTROLLER_H_
