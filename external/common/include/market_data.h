#include "define.h"

struct Market_Data {
  char contract[MAX_CONTRACT_LENGTH];
  double bid_price[PRICE_DEPTH];
  double ask_price[PRICE_DEPTH];
  int bid_size[PRICE_DEPTH];
  int ask_size[PRICE_DEPTH];
  double last_price;
  double volume;
  char tbd[4096];
};
