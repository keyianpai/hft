#ifndef EXTERNAL_COMMON_PRICER_DATA_H_
#define EXTERNAL_COMMON_PRICER_DATA_H_

#include <sys/time.h>
#include "define.h"
#include "market_snapshot.h"

struct PricerData {
  timeval time;
  char topic[MAX_TOPIC_LENGTH];
  char ticker[MAX_TICKER_LENGTH];
  int sequence_no;
  int time_sec;
  double data;
  MarketSnapshot shot;

  void Show(FILE* stream) const {
    timeval show_time;
    gettimeofday(&show_time, NULL);
    fprintf(stream, "%ld %04ld PricerData %s ",
            time.tv_sec, time.tv_usec, ticker);

    fprintf(stream, "%s %d seq_no:%d %lf %ld %04ld\n", topic, time_sec, sequence_no, data, show_time.tv_sec, show_time.tv_usec);

  }
};

#endif  // EXTERNAL_COMMON_PRICER_DATA_H_
