// NetworkServer.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <winsock.h>

#include <iostream>
#include <string>
#include <thread>

#include "global.h"

class Server {
 public:
  Server(int port = 8080);
  ~Server();
  void run();
  void recv(SOCKET &);

 private:
  SOCKET sockfd;
  int port;
  struct sockaddr_in addr;
  void rttTest(struct sockaddr_in &);
  void bandwidthTest(struct sockaddr_in &, int);
};

// TODO: 在此处引用程序需要的其他标头。
