#ifndef SRC_STRAT_TIMECONTROLLER_H_
#define SRC_STRAT_TIMECONTROLLER_H_

#include <market_snapshot.h>
#include <order.h>
#include <sender.h>
#include <exchange_info.h>
#include <order_status.h>
#include <tr1/unordered_map>
#include <sys/time.h>

#include <cmath>
#include <vector>
#include <iostream>
#include <string>

#include <useful_function.h>

class TimeController {
 public:
  TimeController(std::vector<std::string>sleep_time)
    : timezone_diff(8*3600) {
    for (int i = 0; i < sleep_time.size(); i++) {
      std::vector<std::string> tv = SplitStr(sleep_time[i], "-");
      if (tv.size() != 2) {
        printf("split error!size is %u, it's %s\n", tv.size(), sleep_time[i].c_str());
        exit(1);
      }
      if (Translate(tv[0]) >= Translate(tv[1])) {
        printf("time error, stoptime < starttime! it's %s\n", sleep_time[i].c_str());
        exit(1);
      }
      sleep_start.push_back(Translate(tv[0]));
      sleep_stop.push_back(Translate(tv[1]));
    }
  }

  ~TimeController() {
  }

  bool TimeValid(int time) {
    int check_time = time % (24*3600);
    for (int i = 0; i < sleep_start.size(); i++) {
      if (sleep_start[i] <= check_time && check_time <= sleep_stop[i]) {
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

 private:
  int timezone_diff;
  std::vector<int>sleep_start;
  std::vector<int>sleep_stop;
  int Translate(std::string time) {
    std::vector<std::string> content = SplitStr(time, ":");
    if (content.size() != 3) {
      printf("slplit time error, size is %u, it's %s\n", content.size(), time.c_str());
      exit(1);
    }
    int hour = atoi(content[0].c_str());
    int min = atoi(content[1].c_str());
    int sec = atoi(content[2].c_str());
    return hour * 3600 + min * 60 + sec;
  }
};

int main() {
  std::vector<std::string>time;
  time.push_back("22:00:00-23:00:00");
  TimeController tc(time);
  std::cout << tc.TimeValid() << endl;
}

#endif  // SRC_STRAT_TIMECONTROLLER_H_
