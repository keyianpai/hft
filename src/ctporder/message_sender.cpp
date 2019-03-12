#include <string.h>
#include <string>

#include "ctporder/message_sender.h"

MessageSender::MessageSender(CThostFtdcTraderApi* user_api,
                             const std::string & broker_id,
                             const std::string & user_id,
                             const std::string & password,
                             bool use_arbitrage_orders,
                             std::tr1::unordered_map<int, int>*id_map,
                             TokenManager* tm)
  : user_api_(user_api),
    broker_id_(broker_id),
    user_id_(user_id),
    password_(password),
    request_id_(0),
    ctp_order_ref(0),
    use_arbitrage_orders_(use_arbitrage_orders),
    order_id_map(id_map),
    t_m(tm) {
}

void MessageSender::SendLogin() {
  CThostFtdcReqUserLoginField request;
  memset(&request, 0, sizeof(request));

  strncpy(request.BrokerID, broker_id_.c_str(), sizeof(request.BrokerID));
  strncpy(request.UserID, user_id_.c_str(), sizeof(request.UserID));
  strncpy(request.Password, password_.c_str(), sizeof(request.Password));

  printf("Logging in as %s\n", user_id_.c_str());

  int result = user_api_->ReqUserLogin(&request, ++request_id_);
  if (result != 0) {
    printf("SendLogin failed! (%d)\n", result);
    sleep(1);
    throw std::runtime_error("Login failed");
  }
}

void MessageSender::SendSettlementInfoConfirm() {
  CThostFtdcSettlementInfoConfirmField req;
  memset(&req, 0, sizeof(req));

  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));

  int result = user_api_->ReqSettlementInfoConfirm(&req, ++request_id_);
  if (result != 0) {
    printf("SendSettlementInfo failed! (%d)", result);
    sleep(1);
    throw std::runtime_error("SendSettlementInfo failed");
  }
}

void MessageSender::SendQueryTradingAccount() {
  CThostFtdcQryTradingAccountField trading_account;
  memset(&trading_account, 0, sizeof(trading_account));

  strncpy(trading_account.BrokerID, broker_id_.c_str(), sizeof(trading_account.BrokerID));
  strncpy(trading_account.InvestorID, user_id_.c_str(), sizeof(trading_account.InvestorID));

  int result = user_api_->ReqQryTradingAccount(&trading_account, ++request_id_);
  if (result != 0) {
    printf("SendQueryTradingAccount failed! (%d)", result);
    sleep(1);
    throw std::runtime_error("SendQueryTradingAccount failed");
  }
}

void MessageSender::SendQueryInvestorPosition() {
  sleep(1);
  t_m->Init();
  CThostFtdcQryInvestorPositionField investor_position;
  memset(&investor_position, 0, sizeof(investor_position));

  // Query positions for all instruments
  int result;
  for (int retry_count = 0; retry_count < 100; ++retry_count) {
    result = user_api_->ReqQryInvestorPosition(&investor_position, ++request_id_);
    if (result == 0) {
      printf("send query request ok!\n");
      return;
    }
    usleep(50000);
  }

  printf("SendQueryInvestorPosition failed! (%d)", result);
  sleep(1);
  // throw std::runtime_error("SendQueryInvestorPosition failed");
  exit(1);
}

bool MessageSender::Handle(const Order & order) {
  if (order.action == OrderAction::NewOrder) {
    return NewOrder(order);
  }
  if (order.action == OrderAction::ModOrder) {
    CancelOrder(order);
    return true;
  }
  if (order.action == OrderAction::CancelOrder) {
    CancelOrder(order);
    return true;
  }
  if (order.action == OrderAction::QueryPos) {
    order.Show(stdout);
    SendQueryInvestorPosition();
    return true;
  }
  // print error message
  return false;
}

bool MessageSender::NewOrder(const Order& order) {
  t_m->RegisterOrderRef(order);
  CThostFtdcInputOrderField req;
  memset(&req, 0, sizeof(req));

  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));

  strncpy(req.InstrumentID, order.contract, sizeof(req.InstrumentID));

  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", t_m->GetCtpId(order));

  req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
  if (order.side == OrderSide::Buy) {
    req.Direction = THOST_FTDC_D_Buy;
  } else if (order.side == OrderSide::Sell) {
    req.Direction = THOST_FTDC_D_Sell;
  } else {
    return false;
  }

  req.LimitPrice = order.price;
  req.VolumeTotalOriginal = order.size;

  // The exchange requires us to net positions manually.  We track how many "tokens"
  // we have available to net buy/sell orders.  If it is safe to use a close position
  // order, we do so, as it dramatically reduces margin requirements.
  /*
  switch (order_data.position_type) {
    case OrderData::kPositionTypeOpen: req.CombOffsetFlag[0] = THOST_FTDC_OF_Open; break;
    case OrderData::kPositionTypeCloseToday: req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday; break;
    case OrderData::kPositionTypeCloseYesterday: req.CombOffsetFlag[0] = THOST_FTDC_OF_Close; break;
  }
  */
  /*
  if (pos < order.size) {
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
  } else {
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Close_TODAY;
    if (order.side == OrderSide::Buy) {
      (*buy_pos)[order.contract] -= order.size;
    } else {
      (*sell_pos)[order.contract] -= order.size;
    }
  }
  */

  if (use_arbitrage_orders_) {
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Arbitrage;
  } else {
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
  }

  req.TimeCondition = THOST_FTDC_TC_GFD;
  req.VolumeCondition = THOST_FTDC_VC_AV;
  req.ContingentCondition = THOST_FTDC_CC_Immediately;
  req.StopPrice = 0;
  req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
  req.IsAutoSuspend = 0;

  CloseType t = t_m->CheckOffset(order);
  printf("%d %d %d\n", t.yes_size, t.tod_size, t.open_size);

  /*
  if (t.NeedSplit()) {
    printf("%s need split handle, but current handler dont support it!, %d %d %d\n", order.order_ref, t.yes_size, t.tod_size, t.open_size);
    t_m->Restore(order);
    return false;
  }
  */
  req.CombOffsetFlag[0] = t.OffsetFlag;
  int result = user_api_->ReqOrderInsert(&req, ++request_id_);

  printf("SubmitNew %s %s %d@%lf %s %c\n",
    OrderSide::ToString(order.side),
    order.contract,
    order.size,
    order.price,
    order.order_ref,
    req.CombOffsetFlag[0]);

  return result == 0;
}

void MessageSender::CancelOrder(const Order& order) {
  CThostFtdcInputOrderActionField req;
  memset(&req, 0, sizeof(req));

  int ctp_order_ref = t_m->GetCtpId(order);
  Order o = t_m->GetOrder(ctp_order_ref);

  strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
  strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
  // strncpy(req.OrderRef, exchange_id.c_str(), sizeof(req.OrderRef));
  snprintf(req.OrderRef, sizeof(req.OrderRef), "%d", ctp_order_ref);

  strncpy(req.InstrumentID, o.contract, sizeof(req.InstrumentID));

  req.FrontID = front_id_;
  req.SessionID = session_id_;
  req.ActionFlag = THOST_FTDC_AF_Delete;

  user_api_->ReqOrderAction(&req, ++request_id_);

  printf("SubmitCancel %s contract %s\n", o.order_ref, o.contract);
}
