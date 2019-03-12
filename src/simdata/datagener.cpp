#include <iostream>
#include <string>
#include <vector>

#include "simdata/datagener.h"

#define MAXN 10000000

// using namespace std;
DataGener::DataGener(std::string des_name, std::string data_file)
  : file_name(data_file),
    context(NULL),
    socket(NULL) {
  sender = new Sender(des_name.c_str());
  // out_file = fopen("test.txt", "w");
}

DataGener::~DataGener() {
  raw_file.close();
  // fclose(out_file);
  delete sender;
}

void DataGener::Run() {
  // int count = 0;
  std::cout << file_name << endl;
  char buffer[SIZE_OF_SNAPSHOT];
  raw_file.open(file_name.c_str(), ios::in);
  while (raw_file.getline(buffer, SIZE_OF_SNAPSHOT)) {
    // raw_file.getline(buffer, SIZE_OF_SNAPSHOT);
    // std::cout << buffer << endl;
    if (buffer[0] == '\0') {
      break;
    }
    MarketSnapshot snapshot = HandleSnapshot(buffer);
    // HandleSnapshot(buffer);
    char c[2048];
    memcpy(c, &snapshot, sizeof(snapshot));
    sender->Send(snapshot);
    snapshot.Show(stdout);
    /*
    if (count++ % 10000 == 0) {
      snapshot.Show(stdout);
    }
    */
    // std::cout << "send" << c << " out" << endl;
    // sleep(1);
  }
  printf("readfile over!\n");
  raw_file.close();
}
