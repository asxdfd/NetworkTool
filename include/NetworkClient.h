#pragma once

#include <winsock.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

#include "global.h"

class Client {
 public:
  Client(std::string, int);
  ~Client();
  void send();
  void rttTest();
  void bandwidthTest(int, int, uint32_t, char);

 private:
  std::string host;
  int port;
  SOCKET sockfd;
  struct sockaddr_in addr;
  void getBandwidthMsg(char*, int);
};
