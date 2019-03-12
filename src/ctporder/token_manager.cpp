#include "ctporder/token_manager.h"
#include <string>

TokenManager::TokenManager()
    : ctp_id(0) {
  pthread_mutex_init(&token_mutex, NULL);
}

OrderRefStruct TokenManager::SplitOrderRef(std::string orderref) {
  // TODO(nick): check the format
  int num_pos = -1;
  for (unsigned int i = 0; i < orderref.size(); i++) {
    if (isdigit(orderref[i])) {
      num_pos = i;
      break;
    }
  }
  OrderRefStruct os;
  os.strat_name = orderref.substr(0, num_pos);
  os.ref_num = atoi(orderref.substr(num_pos, orderref.size()-num_pos).c_str());
  return os;
}

void TokenManager::Init() {
  buy_token.clear();
  sell_token.clear();
  yes_buy_token.clear();
  yes_sell_token.clear();
  order_id_map.clear();
  ctpid_order_map.clear();
  is_close.clear();
  is_yes_close.clear();
  ref_close.clear();
}

void TokenManager::Restore(Order order) {
  int ctporderref = GetCtpId(order);
  CloseType t = ref_close[ctporderref];
  if (order.side == OrderSide::Buy) {
    pthread_mutex_lock(&token_mutex);
    yes_buy_token[order.contract] += t.yes_size;
    buy_token[order.contract] += t.tod_size;
    pthread_mutex_unlock(&token_mutex);
  } else {
    pthread_mutex_lock(&token_mutex);
    yes_sell_token[order.contract] += t.yes_size;
    sell_token[order.contract] += t.tod_size;
    pthread_mutex_unlock(&token_mutex);
  }
}

void TokenManager::RegisterToken(std::string contract, int num, OrderSide::Enum side) {
  // TODO(nick): add lock
  printf("Register token %s %s %d\n", contract.c_str(), OrderSide::ToString(side), num);
  if (side == OrderSide::Buy) {
    sell_token[contract] += num;
  } else {
    buy_token[contract] += num;
  }
}

void TokenManager::RegisterYesToken(std::string contract, int num, OrderSide::Enum side) {
  // TODO(nick): add lock
  printf("Register yesterday token %s %s %d\n", contract.c_str(), OrderSide::ToString(side), num);
  if (side == OrderSide::Buy) {
    yes_sell_token[contract] += num;
  } else {
    yes_buy_token[contract] += num;
  }
}

void TokenManager::RegisterOrderRef(Order o) {
  OrderRefStruct os = SplitOrderRef(o.order_ref);
  if (os.ref_num == 0) {
    // order_id_map.clear();
    for (std::tr1::unordered_map<std::string, int>::iterator it = order_id_map.begin(); it != order_id_map.end(); it++) {
      OrderRefStruct temp = SplitOrderRef(it->first);
      if (temp.strat_name == os.strat_name) {
        order_id_map.erase(it);
      }
    }
  }
  ctpid_order_map[ctp_id] = o;
  order_id_map[o.order_ref] = ctp_id++;
}

int TokenManager::GetCtpId(Order o) {
  std::tr1::unordered_map<std::string, int>::iterator it = order_id_map.find(o.order_ref);
  if (it == order_id_map.end()) {
    printf("ctporderref not found for %s\n", o.order_ref);
    return -1;
  }
  return it->second;
}

std::string TokenManager::GetOrderRef(int ctp_id) {
  for (std::tr1::unordered_map<std::string, int>::iterator it = order_id_map.begin(); it != order_id_map.end(); it++) {
    if (it->second == ctp_id) {
      return it->first;
    }
  }
  printf("ctpref %d not found!\n", ctp_id);
  return "-1";
}

Order TokenManager::GetOrder(int ctp_order_ref) {
  std::tr1::unordered_map<int, Order>::iterator it = ctpid_order_map.find(ctp_order_ref);
  if (it == ctpid_order_map.end()) {
    printf("order not found for ctpref %d\n", ctp_order_ref);
  }
  return it->second;
}

CloseType TokenManager::CheckOffset(Order order) {
  printf("check offset for order %s: size is %d, side is %s, and token is: buy %d, sell %d, yesbuy %d, yessell %d\n", order.order_ref, order.size, OrderSide::ToString(order.side), buy_token[order.contract], sell_token[order.contract], yes_buy_token[order.contract], yes_sell_token[order.contract]);
  int ctp_order_ref = GetCtpId(order);
  int pos;
  int yes_pos;
  CloseType t;
  if (order.side == OrderSide::Buy) {
    pos = buy_token[order.contract];
    yes_pos = yes_buy_token[order.contract];
  } else if (order.side == OrderSide::Sell) {
    pos = sell_token[order.contract];
    yes_pos = yes_sell_token[order.contract];
  } else {
    printf("token unknown side!\n");
    exit(1);
  }

  if (order.size <= yes_pos) {  // just close_yesterday
    t.yes_size = order.size;
    is_yes_close[ctp_order_ref] = true;
    t.OffsetFlag = THOST_FTDC_OF_CloseYesterday;
  } else {
    if (order.size <= pos) {  // close today
      t.tod_size = order.size;
      is_close[ctp_order_ref] = true;
      t.OffsetFlag = THOST_FTDC_OF_CloseToday;
    } else {  // open
      t.open_size = order.size;
      t.OffsetFlag = THOST_FTDC_OF_Open;
    }
  }
  if (order.side == OrderSide::Buy) {
    pthread_mutex_lock(&token_mutex);
    yes_buy_token[order.contract] -= t.yes_size;
    buy_token[order.contract] -= t.tod_size;
    pthread_mutex_unlock(&token_mutex);
  } else {
    pthread_mutex_lock(&token_mutex);
    yes_sell_token[order.contract] -= t.yes_size;
    sell_token[order.contract] -= t.tod_size;
    pthread_mutex_unlock(&token_mutex);
  }
  ref_close[ctp_order_ref] = t;
  printf("todsize is %d, yessize is %d, opensize is %d\n", t.tod_size, t.yes_size, t.open_size);
  return t;
}

void TokenManager::HandleFilled(Order o) {
  int ctp_order_ref = GetCtpId(o);
  printf("tokenmanager handling filled order %s, isclose is %d\n", o.order_ref, is_close[ctp_order_ref]);
  if (o.side == OrderSide::Buy && !is_close[ctp_order_ref] && !is_yes_close[ctp_order_ref]) {
    pthread_mutex_lock(&token_mutex);
    sell_token[o.contract] += o.size;
    printf("sell token added for %s, now is %d\n", o.contract, sell_token[o.contract]);
    pthread_mutex_unlock(&token_mutex);
  } else if (o.side == OrderSide::Sell && !is_close[ctp_order_ref] && !is_yes_close[ctp_order_ref]) {
    pthread_mutex_lock(&token_mutex);
    buy_token[o.contract] += o.size;
    printf("buy token added for %s, now is %d\n", o.contract, buy_token[o.contract]);
    pthread_mutex_unlock(&token_mutex);
  } else {
    // printf("unknown side!listener 251\n");
  }
}

void TokenManager::HandleCancelled(Order o) {
  int ctp_order_ref = GetCtpId(o);
  printf("tokenmanager handling cancelled order %s, isclose is %d\n", o.order_ref, is_close[ctp_order_ref]);
  if (o.side == OrderSide::Buy) {
    if (is_close[ctp_order_ref]) {
      pthread_mutex_lock(&token_mutex);
      buy_token[o.contract] += o.size;
      printf("cancel buy token added for %s, now is %d\n", o.contract, buy_token[o.contract]);
      pthread_mutex_unlock(&token_mutex);
    } else if (is_yes_close[ctp_order_ref]) {
      pthread_mutex_lock(&token_mutex);
      yes_buy_token[o.contract] += o.size;
      printf("cancel yesbuy token added for %s, now is %d\n", o.contract, yes_buy_token[o.contract]);
      pthread_mutex_unlock(&token_mutex);
    }
  } else if (o.side == OrderSide::Sell) {
    if (is_close[ctp_order_ref]) {
      pthread_mutex_lock(&token_mutex);
      sell_token[o.contract] += o.size;
      printf("cancel sell token added for %s, now is %d\n", o.contract, sell_token[o.contract]);
      pthread_mutex_unlock(&token_mutex);
    } else if (is_yes_close[ctp_order_ref]) {
      pthread_mutex_lock(&token_mutex);
      yes_sell_token[o.contract] += o.size;
      printf("cancel yessell token added for %s, now is %d\n", o.contract, yes_sell_token[o.contract]);
      pthread_mutex_unlock(&token_mutex);
    }
  } else {
    // printf("unknown side!listener 251\n");
  }
}

void TokenManager::PrintToken() {
  printf("****************start print token*******************\n");
  printf("sell token map:\n");
  PrintMap(sell_token);
  printf("buy token map:\n");
  PrintMap(buy_token);
  printf("yes_sell token map:\n");
  PrintMap(yes_sell_token);
  printf("yes_buy token map:\n");
  PrintMap(yes_buy_token);
  printf("*****************end print token********************\n");
}
