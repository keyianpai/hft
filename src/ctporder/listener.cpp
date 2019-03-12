#include <ThostFtdcTraderApi.h>
#include <stdlib.h>
#include <string.h>
#include <tr1/unordered_map>

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "ctporder/message_sender.h"
#include "ctporder/listener.h"

Listener::Listener(const std::string & exchange_info_address,
                   MessageSender* message_sender,
                   const std::string & error_list,
                   std::tr1::unordered_map<int, int>*id_map,
                   TokenManager* tm,
                   bool enable_stdout,
                   bool enable_file)
  : message_sender_(message_sender),
    initialized_(false),
    order_id_map(id_map),
    t_m(tm),
    e_s(enable_stdout),
    e_f(enable_file) {
  /*
  libconfig::Config error_config;
  ReadConfig(error_list, &error_config);

  libconfig::Setting & errors = error_config.lookup("errors");
  for (int i = 0; i < errors.getLength(); ++i) {
    error_list_[errors[i]["value"]] = errors[i]["message"].c_str();
  }
  */
  if (e_f) {
    exchange_file = fopen("ctporder_exchange.txt", "w");
  }
  sender = new Sender(exchange_info_address.c_str());
}

Listener::~Listener() {
  delete sender;
  if (e_f) {
    fclose(exchange_file);
  }
}

void Listener::OnRspError(CThostFtdcRspInfoField *info, int request_id, bool is_last) {
  CheckError("OnRspError", info);
}

void Listener::OnFrontConnected() {
  printf("enter onfrontconnected\n");
  message_sender_->SendLogin();
}

void Listener::OnFrontDisconnected(int reason) {
  printf("Disconnected with reason %d", reason);
}

void Listener::OnHeartBeatWarning(int time_lapse) {
  printf("Time lapse: %d", time_lapse);
}

void Listener::OnRspUserLogin(CThostFtdcRspUserLoginField* user_login,
                              CThostFtdcRspInfoField* info,
                              int request_id,
                              bool is_last) {
  printf("enter onrspuserlogin %d\n", is_last);
  if (is_last) {
    if (CheckError("OnRspUserLogin", info)) {
      printf("%d\n", CheckError("OnRspUserLogin", info));
      sleep(1);
      exit(1);
    }

    front_id_ = user_login->FrontID;
    session_id_ = user_login->SessionID;

    printf("Front id: %d\n", front_id_);
    printf("Session id: %d\n", session_id_);
    printf("Next Order Ref: %d\n", order_ref_);
    printf("here is good\n");

    printf("Login successful\n");

    message_sender_->SetFrontId(front_id_);
    message_sender_->SetSessionId(session_id_);

    // Neccesary to establish ourselves as logged in
    message_sender_->SendQueryTradingAccount();
  }
}

bool Listener::CheckError(const std::string & location,
                          CThostFtdcRspInfoField* info,
                          const std::string & extra_info) {
  bool is_error = info && info->ErrorID != 0;
  if (is_error) {
    printf("Error %d at %s:%s, Msg: %s\n",
      info->ErrorID, location.c_str(), extra_info.c_str(), error_list_[info->ErrorID].c_str());
  }
  return is_error;
}


void Listener::OnRspOrderInsert(CThostFtdcInputOrderField* order,
                                CThostFtdcRspInfoField* info,
                                int request_id,
                                bool is_last) {
  printf("on errinsert for %s\n", order->OrderRef);
  if (CheckError("OnRspOrderInsert", info, order->OrderRef)) {
    ExchangeInfo exchangeinfo;
    exchangeinfo.type = InfoType::Rej;
    int ctp_order_ref = atoi(order->OrderRef);
    Order o = t_m->GetOrder(ctp_order_ref);
    exchangeinfo.side = o.side;
    snprintf(exchangeinfo.reason, sizeof(exchangeinfo.reason), "%d", info->ErrorID);
    snprintf(exchangeinfo.contract, sizeof(exchangeinfo.contract), "%s", o.contract);
    snprintf(exchangeinfo.order_ref, sizeof(exchangeinfo.order_ref), "%s", o.order_ref);
    std::string orderref = t_m->GetOrderRef(ctp_order_ref);
    printf("sent Rej for %s %d back\n", orderref.c_str(), ctp_order_ref);
    if (orderref == "-1") {
      printf("ctporderref %d not found\n", ctp_order_ref);
      return;
    }
    t_m->HandleCancelled(o);
    SendExchangeInfo(exchangeinfo);
    exchangeinfo.Show(stdout);
    /*
    HandleOrder(OrderUpdateReason::Rejected,
                OrderStatus::Rejected,
                order->OrderRef,
                order->Direction,
                0);
    */
    // TODO(nick): print error
  }
}

void Listener::OnErrRtnOrderInsert(CThostFtdcInputOrderField* order,
                                   CThostFtdcRspInfoField *info) {
  printf("on errrtnorderinsert for %s\n", order->OrderRef);
  if (!CheckError("OnErrRtnOrderInsert", info, order->OrderRef)) {
    printf("Got unexpected OnErrRtnOrderInsert");
  }
}

/*
void Listener::HandleFailedCancel() {
}
*/

void Listener::OnRspOrderAction(CThostFtdcInputOrderActionField* order_action,
                                CThostFtdcRspInfoField* info,
                                int request_id,
                                bool is_last) {
  // ErrorID = 26 is a failed cancel
  printf("on errrtnorderaction for %s\n", order_action->OrderRef);
  if (info->ErrorID == 26) {
    // HandleFailedCancel();
    // TODO(nick): handle error
    return;
  }

  if (!CheckError("OnRspOrderAction", info, order_action->OrderRef)) {
    printf("Got unexpected OnRspOrderAction %s", order_action->OrderRef);
  }
}

void Listener::OnErrRtnOrderAction(CThostFtdcOrderActionField* order,
                                   CThostFtdcRspInfoField *info) {
  // Cancel after fill
  printf("on errrtnorderaction for %s\n", order->OrderRef);
  if (info && info->ErrorID == 91) {
    // HandleFailedCancel();
    // TODO(nick): handle error
    return;
  }

  if (!CheckError("OnErrRtnOrderAction", info, order->OrderRef)) {
    // HandleFailedCancel();
    // TODO(nick): handle
  }
}

void Listener::OnRtnOrder(CThostFtdcOrderField* order) {
  if (front_id_ != order->FrontID || session_id_ != order->SessionID) {
    printf("[WARNING] got other client's order: %s %.2f %d %s, don't handle\n",
        order->InstrumentID, order->LimitPrice, order->VolumeTotalOriginal, order->OrderRef);
    printf("front_id %d %d session_id %d %d\n", front_id_, order->FrontID, session_id_, order->SessionID);
    return;
  }

  ExchangeInfo exchangeinfo;
  exchangeinfo.type = InfoType::Unknown;
  int ctp_order_ref = atoi(order->OrderRef);
  Order o = t_m->GetOrder(ctp_order_ref);
  exchangeinfo.side = o.side;
  printf("received onRtnOrder for %d\n", ctp_order_ref);
  std::string orderref = t_m->GetOrderRef(ctp_order_ref);
  if (orderref == "-1") {
    printf("ctporderref %d not found\n", ctp_order_ref);
    return;
  }
  printf("map it into %s\n", orderref.c_str());
  snprintf(exchangeinfo.order_ref, sizeof(exchangeinfo.order_ref), "%s", orderref.c_str());
  snprintf(exchangeinfo.contract, sizeof(exchangeinfo.contract), "%s", o.contract);
  switch (order->OrderSubmitStatus) {
  /////////////////////////////////////////////////
  // Just logging

  case THOST_FTDC_OSS_InsertSubmitted:
    // If an order is filled or pfilled immediately, we won't even get an order
    // accepted callback, just the insert submitted, with one of these statuses.
    if (order->OrderStatus == THOST_FTDC_OST_AllTraded ||
        order->OrderStatus == THOST_FTDC_OST_PartTradedQueueing) {
      exchangeinfo.type = InfoType::Acc;
      break;
    } else if (order->OrderStatus == THOST_FTDC_OST_Canceled) {
      exchangeinfo.type = InfoType::Cancelled;
      break;
    }
    return;

  case THOST_FTDC_OSS_CancelSubmitted:
    // printf("CancelSubmitted %s",
    //   order->OrderRef);
    return;

  /////////////////////////////////////////////////

  case THOST_FTDC_OSS_Accepted:
    if (order->OrderStatus == THOST_FTDC_OST_Canceled) {
      exchangeinfo.type = InfoType::Cancelled;
      t_m->HandleCancelled(o);
    } else {
      exchangeinfo.type = InfoType::Acc;
    }
    break;
  case THOST_FTDC_OSS_InsertRejected:
    if (order->OrderStatus == THOST_FTDC_OST_Canceled &&
        order->ExchangeID[0] == THOST_FTDC_EIDT_CZCE) {
      // This is a FAK miss for ZCE only
      exchangeinfo.type = InfoType::Cancelled;
      t_m->HandleCancelled(o);
    } else {
      printf("InsertRejected %s %c\n",
        order->OrderRef,
        order->OrderStatus);
      exchangeinfo.type = InfoType::Rej;
      t_m->HandleCancelled(o);
    }
    break;
  case THOST_FTDC_OSS_CancelRejected:
    printf("CancelRejected %s",
      order->OrderRef);
    exchangeinfo.type = InfoType::CancelRej;
    return;
  }

  if (exchangeinfo.type == InfoType::Unknown) {
    printf("OnRtnOrder did not set type!\n");
    return;
  }

  SendExchangeInfo(exchangeinfo);
  if (e_s) {
    exchangeinfo.Show(stdout);
  }
  if (e_f) {
    exchangeinfo.Show(exchange_file);
  }

  printf("OrderUpdate Id:%s %c %c VolTraded:%d VolTot:%d\n",
    order->OrderRef,
    order->OrderStatus,
    order->OrderSubmitStatus,
    order->VolumeTraded,
    order->VolumeTotal);
}

void Listener::OnRtnTrade(CThostFtdcTradeField* trade) {
  /*
  OrderSide::Enum side;
  if (trade->Direction == THOST_FTDC_D_Buy) {
    side = OrderSide::Buy;
  } else if (trade->Direction == THOST_FTDC_D_Sell) {
    side = OrderSide::Sell;
  } else {
    side = OrderSide::Buy;
    printf("Unexpected OrderSide!");
    return;
  }
  */

  ExchangeInfo exchangeinfo;
  int ctp_order_ref = atoi(trade->OrderRef);
  std::string orderref = t_m->GetOrderRef(ctp_order_ref);
  Order o = t_m->GetOrder(ctp_order_ref);
  exchangeinfo.side = o.side;
  if (orderref == "-1") {
    printf("trade ctporderref %d not found\n", ctp_order_ref);
    return;
  }

  snprintf(exchangeinfo.contract, sizeof(exchangeinfo.contract), "%s", o.contract);
  snprintf(exchangeinfo.order_ref, sizeof(exchangeinfo.order_ref), "%s", orderref.c_str());
  printf("received onRtnTrade for %d, and map it into %s\n", ctp_order_ref, orderref.c_str());

  t_m->HandleFilled(o);
  exchangeinfo.type = InfoType::Filled;
  exchangeinfo.trade_price = trade->Price;
  exchangeinfo.trade_size = trade->Volume;
  if (e_s) {
    exchangeinfo.Show(stdout);
  }
  if (e_f) {
    exchangeinfo.Show(exchange_file);
  }
  SendExchangeInfo(exchangeinfo);

  printf("Order %s executed %d at %lf\n",
    trade->OrderRef,
    trade->Volume,
    trade->Price);
}

void Listener::OnRspQryTradingAccount(CThostFtdcTradingAccountField* trading_account,
                                      CThostFtdcRspInfoField* info,
                                      int request_id,
                                      bool is_last) {
  message_sender_->SendSettlementInfoConfirm();
}

void Listener::OnRspSettlementInfoConfirm(
  CThostFtdcSettlementInfoConfirmField* settlement_info,
  CThostFtdcRspInfoField* info,
  int request_id,
  bool is_last) {
  // Now, query all our outstanding positions so we can figure out if we
  // can net orders.
  // message_sender_->SendQueryInvestorPosition();
}

void Listener::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* investor_position,
                                        CThostFtdcRspInfoField* info,
                                        int request_id,
                                        bool is_last) {
  if (CheckError("OnRspQryInvestorPosition", info)) {
    return;
  }

  // printf("%s, Ydposition is %d, position is %d, positioncost is %lf, opencost is %lf\n", investor_position->InstrumentID, investor_position->YdPosition, investor_position->Position, investor_position->PositionCost, investor_position->OpenCost);

  /*
  if (initialized_) {
    return;
  }
  */

  if (investor_position) {
    bool result = true;
    // init to ignore compile error
    OrderSide::Enum side = OrderSide::Buy;
    if (investor_position->PosiDirection == THOST_FTDC_PD_Long) {
      side = OrderSide::Buy;
    } else if (investor_position->PosiDirection == THOST_FTDC_PD_Short) {
      side = OrderSide::Sell;
    } else {
      printf("Got unexpected position: %s %c %d\n", investor_position->InstrumentID, investor_position->PosiDirection, investor_position->Position);
      result = false;
      // return;
    }

    const std::string & symbol = investor_position->InstrumentID;
    if (result) {
      ExchangeInfo info;
      snprintf(info.contract, sizeof(info.contract), "%s", symbol.c_str());
      // info.trade_price = (investor_position->PositionCost*investor_position->YdPosition + investor_position->OpenCost*investor_position->Position) / (investor_position->Position+investor_position->YdPosition);
      if (investor_position->YdPosition > 0 && investor_position->PositionCost > 0.1) {
        t_m->RegisterYesToken(symbol, investor_position->YdPosition, side);
        info.trade_size = (investor_position->PosiDirection == THOST_FTDC_PD_Long)?investor_position->YdPosition:-investor_position->YdPosition;
      } else if (investor_position->YdPosition == 0 && investor_position->PositionCost > 0.1) {
        t_m->RegisterToken(symbol, investor_position->Position, side);
        info.trade_size = (investor_position->PosiDirection == THOST_FTDC_PD_Long)?investor_position->Position:-investor_position->Position;
      } else {
        info.trade_size = 0;
      }
      info.trade_price = investor_position->PositionCost/abs(info.trade_size);
      info.type = InfoType::Position;
      SendExchangeInfo(info);
      info.Show(stdout);
      // printf("opencost is %lf, openamount is %lf marginrate is %lf %lf\n", investor_position->OpenCost, investor_position->OpenAmount, investor_position->MarginRateByMoney, investor_position->MarginRateByVolume);
    }
  }

  if (is_last) {
    initialized_ = true;
    ExchangeInfo endinfo;
    snprintf(endinfo.contract, sizeof(endinfo.contract), "%s", "positionend");
    endinfo.type = InfoType::Position;
    endinfo.trade_price = 0.0;
    endinfo.trade_size = 0;
    SendExchangeInfo(endinfo);
    endinfo.Show(stdout);
    printf("Received all positions - safe to trade\n");
    t_m->PrintToken();
  }
}

void Listener::SendExchangeInfo(const ExchangeInfo & info) {
  printf("Sending ExchangeInfo\n");
  sender->Send(info);
}
