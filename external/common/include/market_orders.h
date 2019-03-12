#include "define.h"
#include "order_side.h"

struct Order {
  char contract[MAX_CONTRACT_LENGTH];
  double price;
  int size;
  OrderSide::Enum side;
  char tbd[4096];
};
