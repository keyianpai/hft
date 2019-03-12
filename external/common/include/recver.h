#ifndef RECVER_H_
#define RECVER_H_

#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include "exchange_info.h"
#include "market_snapshot.h"
#include "pricer_data.h"
#include "define.h"
#include "order.h"

using namespace std;

class Recver {
 public:
  Recver(string name, string mode = "ipc");

  ~Recver();

  MarketSnapshot Recv(MarketSnapshot shot);

  Order Recv(Order order);

  ExchangeInfo Recv(ExchangeInfo i);

  PricerData Recv(PricerData p);

 private:
  zmq::context_t* con;
  zmq::socket_t* sock;
};

#endif  //  RECVER_H_
