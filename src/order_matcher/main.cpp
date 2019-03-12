#include <stdio.h>
#include <zmq.hpp>
#include <recver.h>
#include <tr1/unordered_map>

#include <iostream>
#include <string>

#include "order_matcher/order_handler.h"

void* RunOrderCommandListener(void *param) {
  OrderHandler* order_handler = reinterpret_cast<OrderHandler*>(param);
  Recver* r = new Recver("order_pub");
  while (true) {
    Order o;
    o = r->Recv(o);
    if (!order_handler->Handle(o)) {
      // handle error
    }
  }
  return NULL;
}

int main() {
  OrderHandler order_handler;

  pthread_t order_thread;
  if (pthread_create(&order_thread,
                     NULL,
                     &RunOrderCommandListener,
                     &order_handler) != 0) {
    perror("pthread_create");
    exit(1);
  }
  while (true) {
  }
}
