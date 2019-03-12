#include <stdio.h>

#include <iostream>
#include <string>

#include "simorder/orderlistener.h"

orderlistener::orderlistener(std::string ipc_name) {
  /*
  con = new zmq::context_t(1);
  sock = new zmq::socket_t(*con, ZMQ_SUB);
  sock->setsockopt(ZMQ_SUBSCRIBE, 0, 0);
  std::string name = "ipc://";
  name += ipc_name;
  sock->connect(name);
  sleep(1);
  */
  recver = new Recver(ipc_name.c_str());
}

orderlistener::~orderlistener() {
  delete recver;
}

void orderlistener::Run() {
  while (true) {
    /*
    char buffer[2048];
    sock->recv(buffer, sizeof(buffer));
    Order* o = reinterpret_cast<Order*>(buffer);
    */
    Order o;
    o = recver->Recv(o);
    // printf("recv order request: %s %d@%lf %s\n", o.contract, o.size, o.price, OrderSide::ToString(o.side));
    o.Show(stdout);
  }
}
