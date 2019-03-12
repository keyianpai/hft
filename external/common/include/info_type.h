#ifndef INFO_TYPE_H_
#define INFO_TYPE_H_

struct InfoType {
  enum Enum {
    Uninited,
    Acc,
    Rej,
    Cancelled,
    CancelRej,
    Filled,
    Pfilled,
    Position,
    Unknown
  };
  static inline const char* ToString(Enum type) {
    if (type == Uninited)
      return "Uninited";
    if (type == Acc)
      return "Accepted";
    if (type == Rej)
      return "Rejected";
    if (type == Cancelled)
      return "Cancelled";
    if (type == CancelRej)
      return "CancelRej";
    if (type == Filled)
      return "Filled";
    if (type == Pfilled)
      return "Pfilled";
    if (type == Position)
      return "Position";
    return "unknown_infotype";
  }
};

#endif  //  INFO_TYPE_H_
