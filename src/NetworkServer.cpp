// NetworkTool.cpp: 定义应用程序的入口点。
//

#include "NetworkServer.h"

#include "cxxopts.hpp"

Server::Server(int port) {
  this->port = port;

  int Ret;
  WSADATA wsaData;
  if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
    std::cout << "WSAStartup() failed with error " << Ret << std::endl;
    WSACleanup();
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    std::cout << "bind error." << std::endl;
    exit(1);
  }
}

Server::~Server() {
  closesocket(sockfd);
  WSACleanup();
}

void Server::rttTest(struct sockaddr_in &client_addr) {
  std::cout << "client start rtt test" << std::endl;
  int len = sizeof(client_addr);
  char buf[1024] = {0};
  memset(buf, 0, sizeof(buf));
  recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &len);

  int times = std::stoi(buf);
  for (int i = 0; i < times; i++) {
    memset(buf, 0, sizeof(buf));
    recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr,
             &len);
    sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, len);
  }
  std::cout << "client complete rtt test" << std::endl;
}

void Server::bandwidthTest(struct sockaddr_in &client_addr, int testSeconds) {
  int len = sizeof(client_addr);
  char buf[1024] = {0};
  memset(buf, 0, sizeof(buf));

  std::cout << "client start bandwidth test" << std::endl;
  int total_len = 0;
  int total_pkt = 0;
  int recv_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                          (struct sockaddr *)&client_addr, &len);
  auto start = std::chrono::high_resolution_clock::now();
  total_len += recv_len;
  total_pkt += 1;
  long long timestampMsg = std::stoll(Util::split(buf, ',').at(0));
  long long timestamp = Util::getTimestamp();
  int delay = timestamp - timestampMsg;
  int max_delay = delay;
  int min_delay = delay;

  std::chrono::steady_clock::time_point lastArrival;
  while (true) {
    auto now = std::chrono::high_resolution_clock::now();
    auto time_used =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();
    if (time_used > testSeconds * 1000 + 5000) {
      std::cout << "test time out" << std::endl;
      break;
    }

    memset(buf, 0, sizeof(buf));
    recv_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                        (struct sockaddr *)&client_addr, &len);
    std::string str = buf;
    if (recv_len > 0) {
      if (str.substr(0, 3) == "end") {
        std::cout << "client complete bandwidth test" << std::endl;
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(
                        lastArrival - start)
                        .count();
        int bandwidth = total_len * 1000 / time;
        int jitter = max_delay - min_delay;
        int pktLoss =
            std::stoi(Util::split(str.c_str(), ',').at(1)) - total_pkt;
        std::cout << "bandwidth: " << bandwidth << " bytes/s" << std::endl
                  << "jitter: " << jitter << "ms" << std::endl
                  << "packet loss: " << pktLoss << std::endl;
        std::string reply = std::to_string(bandwidth) + "," +
                            std::to_string(jitter) + "," +
                            std::to_string(pktLoss);
        sendto(sockfd, reply.c_str(), reply.length(), 0,
               (struct sockaddr *)&client_addr, len);
        break;
      } else {
        lastArrival = std::chrono::high_resolution_clock::now();
        timestamp = Util::getTimestamp();
        total_len += recv_len;
        total_pkt += 1;

        timestampMsg = std::stoll(Util::split(buf, ',').at(0));
        delay = timestamp - timestampMsg;
        if (delay > max_delay) max_delay = delay;
        if (delay < min_delay) min_delay = delay;
      }
    } else {
      std::cout << "recv msg error" << std::endl;
      break;
    }
  }
}

void Server::run() {
  char buf[1024] = {0};
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  int len = sizeof(client_addr);
  int recv_len = 0;

  while (1) {
    memset(buf, 0, sizeof(buf));
    recv_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                        (struct sockaddr *)&client_addr, &len);
    std::cout << inet_ntoa(client_addr.sin_addr) << ": " << buf << std::endl;
    std::string str = buf;
    if (str == "rtt") {
      char reply[] = "ok";
      sendto(sockfd, reply, sizeof(reply), 0, (struct sockaddr *)&client_addr,
             len);
      rttTest(client_addr);
    } else if (str.substr(0, 9) == "bandwidth") {
      char reply[] = "ok";
      sendto(sockfd, reply, sizeof(reply), 0, (struct sockaddr *)&client_addr,
             len);
      bandwidthTest(client_addr, std::stoi(str.substr(10)));
    }
  }
}

cxxopts::ParseResult parse(int argc, char *argv[]) {
  try {
    cxxopts::Options options(argv[0], " - example command line options");

    options.add_options()("p,port", "server port", cxxopts::value<int>());

    auto result = options.parse(argc, argv);

    return result;

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  auto result = parse(argc, argv);

  if (result.count("p")) {
    int port = result["p"].as<int>();
    Server server(port);
    server.run();
  } else {
    Server server;
    server.run();
  }

  return 0;
}
