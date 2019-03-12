#ifndef ORDER_H_
#define ORDER_H_

#include "define.h"
#include "order_side.h"
#include "order_action.h"
#include "order_status.h"
#include "offset.h"

#include <stdio.h>
#include <sys/time.h>

struct Order {
  timeval wrap_time;
  char contract[MAX_CONTRACT_LENGTH];
  double price;
  int size;
  int traded_size;
  OrderSide::Enum side;
  char order_ref[MAX_ORDERREF_SIZE];
  OrderAction::Enum action;
  OrderStatus::Enum status;
  Offset::Enum offset;
  char tbd[128];

  Order()
    : size(0),
      traded_size(0) {
    snprintf(tbd, sizeof(tbd), "%s", "null");
  }

  bool Valid() {
    if (status == OrderStatus::SubmitNew || status == OrderStatus::New) {
      return true;
    }
    return false;
  }
  void Show(FILE* stream) const {
    timeval show_time;
    gettimeofday(&show_time, NULL);
    fprintf(stream, "%ld %04ld %ld %04ld Order %s |",
            show_time.tv_sec, show_time.tv_usec, wrap_time.tv_sec, wrap_time.tv_usec, contract);

      fprintf(stream, " %lf@%d %d %s %s %s %s %s %s\n", price, size, traded_size, OrderSide::ToString(side), order_ref, OrderAction::ToString(action), OrderStatus::ToString(status), Offset::ToString(offset), tbd);
  }
};

#endif  //  ORDER_H_
