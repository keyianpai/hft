#ifndef EXCHANGE_INFO_H_
#define EXCHANGE_INFO_H_

#include <sys/time.h>
#include "define.h"
#include "info_type.h"
#include "order_side.h"

struct ExchangeInfo {
  InfoType::Enum type;
  char contract[MAX_CONTRACT_LENGTH];
  char order_ref[MAX_ORDERREF_SIZE];
  int trade_size;
  double trade_price;
  char reason[EXCHANGE_INFO_SIZE];
  OrderSide::Enum side;

  ExchangeInfo()
    : trade_size(0),
      trade_price(-1) {
  }

  void Show(FILE* stream) const {
    timeval time;
    gettimeofday(&time, NULL);
    fprintf(stream, "%ld %06ld exchangeinfo %s |",
            time.tv_sec, time.tv_usec, order_ref);

    fprintf(stream, " %lf@%d %s %s %s\n", trade_price, trade_size, InfoType::ToString(type), contract, OrderSide::ToString(side));
  }
};

#endif  //  EXCHANGE_INFO_H_
