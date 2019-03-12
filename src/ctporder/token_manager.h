#ifndef SRC_CTPORDER_TOKEN_MANAGER_H_
#define SRC_CTPORDER_TOKEN_MANAGER_H_

#include <order.h>
#include <stdio.h>
#include <stdlib.h>
#include <ThostFtdcUserApiDataType.h>
#include <common_tools.h>

#include <tr1/unordered_map>
#include <string>
#include <cctype>

struct OrderRefStruct {
  std::string strat_name;
  int ref_num;
};

struct CloseType {
  int yes_size;
  int tod_size;
  int open_size;
  int OffsetFlag;
  CloseType()
    : yes_size(0),
      tod_size(0),
      open_size(0),
      OffsetFlag(THOST_FTDC_OF_Open) {
  }
  bool NeedSplit() {
    if (yes_size*tod_size > 0 || yes_size*open_size > 0 || tod_size*open_size > 0) {
      return true;
    }
    if (yes_size > 0) {
      OffsetFlag = THOST_FTDC_OF_CloseYesterday;
    } else if (tod_size > 0) {
      OffsetFlag = THOST_FTDC_OF_CloseToday;
    } else if (open_size > 0) {
      OffsetFlag = THOST_FTDC_OF_Open;
    } else {
      printf("wrong size for close type!%d %d %d\n", yes_size, tod_size, open_size);
      return true;
    }
    return false;
  }
};

class TokenManager {
 public:
  TokenManager();

  OrderRefStruct SplitOrderRef(std::string orderref);

  void Init();

  void RegisterToken(std::string contract, int num, OrderSide::Enum side);
  void RegisterYesToken(std::string contract, int num, OrderSide::Enum side);
  void RegisterOrderRef(Order o);

  int GetCtpId(Order o);

  std::string GetOrderRef(int ctp_id);

  Order GetOrder(int ctp_order_ref);

  CloseType CheckOffset(Order order);
  void Restore(Order order);

  void HandleFilled(Order o);

  void HandleCancelled(Order o);
  void PrintToken();

 private:
  std::tr1::unordered_map<std::string, int> buy_token;
  std::tr1::unordered_map<std::string, int> sell_token;
  std::tr1::unordered_map<std::string, int> yes_buy_token;
  std::tr1::unordered_map<std::string, int> yes_sell_token;
  std::tr1::unordered_map<std::string, int> order_id_map;
  std::tr1::unordered_map<int, Order> ctpid_order_map;
  std::tr1::unordered_map<int, bool> is_close;
  std::tr1::unordered_map<int, bool> is_yes_close;
  std::tr1::unordered_map<int, CloseType> ref_close;
  pthread_mutex_t token_mutex;
  int ctp_id;
};

#endif  // SRC_CTPORDER_TOKEN_MANAGER_H_
