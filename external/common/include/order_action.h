#ifndef ORDER_ACTION_H_
#define ORDER_ACTION_H_

struct OrderAction {
  enum Enum {
    Uninited,
    NewOrder,
    ModOrder,
    CancelOrder,
    QueryPos,
    PlainText
  };

  static inline const char* ToString(Enum action) {
    if (action == OrderAction::Uninited)
      return "Uninited";
    if (action == OrderAction::NewOrder)
      return "new_order";
    if (action == OrderAction::ModOrder)
      return "replace_order";
    if (action == OrderAction::CancelOrder)
      return "cancel_order";
    if (action == OrderAction::QueryPos)
      return "query_pos";
    if (action == OrderAction::PlainText)
      return "plaintext";
    return "error_unknown_order";
  }
};

#endif  // ORDER_ACTION_H_
