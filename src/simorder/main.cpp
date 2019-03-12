#include <stdio.h>
#include <zmq.hpp>

#include <iostream>

#include "simorder/orderlistener.h"

int main() {
  orderlistener oh("order");
  oh.Run();
}
