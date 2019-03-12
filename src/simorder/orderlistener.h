#ifndef SRC_SIMORDER_ORDERLISTENER_H_
#define SRC_SIMORDER_ORDERLISTENER_H_

#include <stdio.h>
#include <zmq.hpp>
#include <order.h>
#include <recver.h>

#include <iostream>
#include <string>

class orderlistener {
 public:
  explicit orderlistener(std::string ipc_name);
  ~orderlistener();
  void Run();

 private:
  Recver* recver;
};

#endif  // SRC_SIMORDER_ORDERLISTENER_H_
