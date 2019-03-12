#ifndef SRC_SIMDATA_DATAGENER_H_
#define SRC_SIMDATA_DATAGENER_H_

#include <zmq.hpp>
#include <sender.h>
#include <market_snapshot.h>
#include <common_tools.h>
#include <define.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <string>

class DataGener {
 public:
  DataGener(std::string shm_name, std::string data_file);
  ~DataGener();
  void Run();

 private:
  // MarketSnapshot HandleSnapshot(std::string raw_shot);
  std::ifstream raw_file;
  std::string file_name;
  zmq::context_t* context;
  zmq::socket_t* socket;
  Sender* sender;
  FILE* out_file;
};

#endif  // SRC_SIMDATA_DATAGENER_H_
