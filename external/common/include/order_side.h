#ifndef ORDER_SIDE_H_
#define ORDER_SIDE_H_

struct OrderSide {
  enum Enum {
    Unknown,
    Buy,
    Sell
  };

  OrderSide::Enum ReverseSide(Enum side) {
    if (side == OrderSide::Buy) {
      return OrderSide::Sell;
    } else if (side == OrderSide::Sell) {
      return OrderSide::Buy;
    } else {  // TODO: handle error
      return OrderSide::Buy;
    }
  }
  static inline const char* ToString(Enum side) {
    if (side == OrderSide::Buy) {
      return "BUY";
    } else if (side == OrderSide::Sell) {
      return "SELL";
    } else if (side == OrderSide::Unknown) {
      return "Unknown";
    }
    return "UNKNOWN_SIDE";
  }
};

#endif  // ORDER_SIDE_H_
