#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <recver.h>
#include <sender.h>
#include <market_snapshot.h>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>

int main() {
  Recver recver("data_source");
  Sender sender("data_sub", "connect");
  while (true) {
    MarketSnapshot shot;
    shot = recver.Recv(shot);
    sender.Send(shot);
    // shot.Show(stdout);
  }
}
