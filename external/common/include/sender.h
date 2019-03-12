#ifndef SENDER_H_
#define SENDER_H_

#include <zmq.hpp>
#include <unistd.h>
#include <string>
#include "exchange_info.h"
#include "market_snapshot.h"
#include "pricer_data.h"
#include "define.h"
#include "order.h"

using namespace std;

class Sender {
 public:
  Sender(string name, std::string bs_mode = "bind", std::string mode = "ipc");

  ~Sender();

  void Send(MarketSnapshot shot);
  void Send(Order order);
  void Send(ExchangeInfo info);
  void Send(PricerData p);
  void Send(const char* s);

 private:
  zmq::context_t* con;
  zmq::socket_t* sock;
  pthread_mutex_t mutex;
};

#endif // SENDER_H_
