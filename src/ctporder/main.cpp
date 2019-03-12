#include <stdio.h>
#include <zmq.hpp>
#include <recver.h>
#include <ThostFtdcTraderApi.h>
#include <tr1/unordered_map>

#include <iostream>
#include <string>

#include "ctporder/message_sender.h"
#include "ctporder/listener.h"
#include "ctporder/token_manager.h"

FILE* order_file;
bool enable_stdout = true;
bool enable_file = true;

void* RunOrderCommandListener(void *param) {
  // Recver* r = reinterpret_cast<Recver*>(param);
  MessageSender* message_sender = reinterpret_cast<MessageSender*>(param);
  Recver* r = new Recver("order_pub");
  while (true) {
    Order o;
    o = r->Recv(o);
    if (enable_stdout) {
      o.Show(stdout);
    }
    if (enable_file) {
      o.Show(order_file);
    }
    // check order's correct
    if (!message_sender->Handle(o)) {
      // handle error
    }
  }
  return NULL;
}

int main() {
  enable_file = true;
  enable_stdout = true;
  if (enable_file) {
    order_file = fopen("ctporder_order.txt", "w");
  }
  CThostFtdcTraderApi* user_api = CThostFtdcTraderApi::CreateFtdcTraderApi();

  std::string broker = "9999";
  std::string username = "116909";
  std::string password = "yifeng";
  /*
  std::string broker = "9999";
  std::string username = "115686";
  std::string password = "fz567789";
  */
  tr1::unordered_map<int, int> order_id_map;

  TokenManager tm;
  MessageSender message_sender(user_api,
                               broker,
                               username,
                               password,
                               false,
                               &order_id_map,
                               &tm);

  Listener listener("exchange_info",
                    &message_sender,
                    "error_list",
                    &order_id_map,
                    &tm,
                    enable_stdout,
                    enable_file);

  pthread_t order_thread;
  if (pthread_create(&order_thread,
                     NULL,
                     &RunOrderCommandListener,
                     &message_sender) != 0) {
    perror("pthread_create");
    exit(1);
  }

  user_api->RegisterSpi(&listener);
  printf("register spi sent\n");

  user_api->SubscribePrivateTopic(THOST_TERT_QUICK);
  user_api->SubscribePublicTopic(THOST_TERT_QUICK);
  std::string counterparty_host = "tcp://180.168.146.187:10000";
  user_api->RegisterFront(const_cast<char*>(counterparty_host.c_str()));
  user_api->Init();
  if (enable_file) {
    fclose(order_file);
  }
  while (true) {
  }
  user_api->Release();
}
