#ifndef SRC_CTPORDER_MESSAGE_SENDER_H_
#define SRC_CTPORDER_MESSAGE_SENDER_H_

#include <ThostFtdcTraderApi.h>
#include <order_side.h>
#include <order.h>
#include <tr1/unordered_map>

#include <stdexcept>
#include <string>

#include "ctporder/token_manager.h"

class MessageSender {
 public:
  MessageSender(CThostFtdcTraderApi* user_api,
                const std::string & broker_id,
                const std::string & user_id,
                const std::string & password,
                bool use_arbitrage_orders,
                std::tr1::unordered_map<int, int>*id_map,
                TokenManager* tm);

  void SetFrontId(int front_id) { front_id_ = front_id; }
  void SetSessionId(int session_id) { session_id_ = session_id; }

  void SendLogin();
  void SendQueryTradingAccount();
  void SendSettlementInfoConfirm();
  void SendQueryInvestorPosition();

  bool Handle(const Order & order);

  bool NewOrder(const Order& order);
  bool ModOrder(const Order& order);

 private:
  void CancelOrder(const Order& order);

  CThostFtdcTraderApi* user_api_;

  std::string broker_id_;
  std::string user_id_;
  std::string password_;

  int request_id_;
  int front_id_;
  int session_id_;

  int ctp_order_ref;
  bool use_arbitrage_orders_;
  std::tr1::unordered_map<int, int>* order_id_map;  // strat id vs ctp id
  std::tr1::unordered_map<int, Order> order_map;  // ctp id vs Order
  TokenManager* t_m;
};

#endif  // SRC_CTPORDER_MESSAGE_SENDER_H_
