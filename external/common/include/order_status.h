#ifndef ORDER_STATUS_H_
#define ORDER_STATUS_H_

struct OrderStatus {
  enum Enum {
    Uninited,
    SubmitNew,
    New,
    Rejected,
    Modifying,
    Cancelling,
    Cancelled,
    CancelRej,
    Pfilled,
    Filled,
    Sleep
  };

  static inline const char* ToString(Enum status) {
    switch (status) {
     case Uninited:
       return "Uninited";
       break;
     case SubmitNew:
       return "SubmitNew";
       break;
     case New:
       return "New";
       break;
     case Rejected:
       return "Rejected";
       break;
     case Modifying:
       return "Modifying";
       break;
     case Cancelling:
       return "Cancelling";
       break;
     case Cancelled:
       return "Cancelled";
       break;
     case CancelRej:
       return "CancelRej";
       break;
     case Pfilled:
       return "Pfilled";
       break;
     case Filled:
       return "Filled";
       break;
     case Sleep:
       return "Sleep";
       break;
     default:
       return "Unknown";
       break;
    }
  }
};

#endif  // ORDER_STATUS_H_
