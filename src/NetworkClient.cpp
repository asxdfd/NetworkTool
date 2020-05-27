#include "NetworkClient.h"

#include "cxxopts.hpp"

Client::Client(std::string host, int port) : host(host), port(port) {
  int Ret;
  WSADATA wsaData;
  if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
    std::cout << "WSAStartup() failed with error " << Ret << std::endl;
    WSACleanup();
  }

  this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
  addr.sin_port = htons(port);
}

void Client::send() {
  char send_buf[1024] = {0};
  char recv_buf[1024] = {0};
  int rc = 0;
  int len = sizeof(addr);
  while (1) {
    memset(send_buf, 0, sizeof(send_buf));
    memset(recv_buf, 0, sizeof(recv_buf));
    std::cin.getline(send_buf, 1024);
    if (send_buf[0] == '0') break;
    //·¢ËÍUDP
    rc = sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&addr,
                len);
    recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr,
             &len);
    std::cout << "server: " << recv_buf << std::endl;
  }
}

void Client::rttTest() {
  std::string send_str = "rtt";
  char recv_buf[1024] = {0};
  int len = sizeof(addr);
  memset(recv_buf, 0, sizeof(recv_buf));
  sendto(sockfd, send_str.c_str(), send_str.length(), 0,
         (struct sockaddr*)&addr, len);
  recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr,
           &len);

  std::string recv_str = recv_buf;
  if (recv_str == "ok") {
    srand(time(0));
    int times = rand() % 20 + 1;
    std::cout << "start rtt test." << std::endl;
    std::string time_str = std::to_string(times);
    sendto(sockfd, time_str.c_str(), sizeof(time_str.c_str()), 0,
           (struct sockaddr*)&addr, sizeof(addr));

    int total = 0;

    for (int i = 0; i < times; i++) {
      memset(recv_buf, 0, sizeof(recv_buf));
      char testMsg[] = "test rtt";
      auto start = std::chrono::high_resolution_clock::now();
      sendto(sockfd, testMsg, sizeof(testMsg), 0, (struct sockaddr*)&addr, len);
      recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr,
               &len);
      auto end = std::chrono::high_resolution_clock::now();
      auto time_used =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
              .count();

      std::cout << "rtt test, udp msg " << (i + 1)
                << ", time used: " << (time_used / 1000.0) << "ms" << std::endl;
      total += time_used;
    }
    std::cout << "rtt test complete. rtt: " << (total / times / 1000.0) << "ms"
              << std::endl;
  } else {
    std::cout << "server rejected test." << std::endl;
    return;
  }
}

void Client::bandwidthTest(int testSeconds, int packetSize, uint32_t bandwidth,
                           char bandwidthUnit) {
  uint64_t bandwidthValue = 0;
  switch (bandwidthUnit) {
    case 'b':
    case 'B':
      bandwidthValue = bandwidth;
      break;
    case 'k':
    case 'K':
      bandwidthValue = 1024 * (uint64_t)bandwidth;
      break;
    case 'm':
    case 'M':
      bandwidthValue = 1024 * 1024 * (uint64_t)bandwidth;
      break;
    case 'g':
    case 'G':
      bandwidthValue = 1024 * 1024 * 1024 * (uint64_t)bandwidth;
      break;
    default:
      throw std::runtime_error("bandwidthUnit error: " +
                               std::to_string(bandwidthUnit));
  }

  std::string send_str = "bandwidth," + std::to_string(testSeconds);
  char recv_buf[1024] = {0};
  int len = sizeof(addr);
  memset(recv_buf, 0, sizeof(recv_buf));
  sendto(sockfd, send_str.c_str(), send_str.length(), 0,
         (struct sockaddr*)&addr, len);
  recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&addr,
           &len);

  std::string recv_str = recv_buf;
  if (recv_str == "ok") {
    std::cout << "start bandwidth test, test sec: " << testSeconds
              << "s, bandwidth value: " << bandwidthValue << "bytes."
              << std::endl;

    int groupTime = 100;
    int packets = bandwidthValue / packetSize;
    int packetsPerSecond = packets / testSeconds;
    int packetsPerGroup =
        std::ceil((double)packetsPerSecond * groupTime / 1000);
    double packetsIntervalMs = (double)1000 / packetsPerSecond;

    auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto time_used =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
            .count();

    int seq = 0;
    char send_buf[1024] = {0};
    while (time_used < testSeconds * 1000) {
      for (int i = 0; i < packetsPerGroup; i++) {
        memset(send_buf, 0, sizeof(send_buf));
        getBandwidthMsg(send_buf, packetSize);
        sendto(sockfd, send_buf, packetSize, 0, (struct sockaddr*)&addr, len);
        seq++;
      }
      now = std::chrono::high_resolution_clock::now();
      time_used =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
              .count();
      long aheadTime = (seq * packetsIntervalMs) - time_used;
      if (aheadTime > 5) {
        std::this_thread::sleep_for(std::chrono::milliseconds(aheadTime - 2));
      }
    }

    std::string end_msg = "end," + std::to_string(seq);
    sendto(sockfd, end_msg.c_str(), end_msg.length(), 0,
           (struct sockaddr*)&addr, len);

    memset(recv_buf, 0, sizeof(recv_buf));
    int recv_len = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
                            (struct sockaddr*)&addr, &len);
    if (recv_len > 0) {
      std::cout << "bandwidth test complete" << std::endl;
      std::vector<std::string> report = Util::split(recv_buf, ',');
      int b = std::stoi(report.at(0));
      int j = std::stoi(report.at(1));
      int l = std::stoi(report.at(2));
      std::cout << "bandwidth: " << b << " bytes/s" << std::endl
                << "jitter: " << j << "ms" << std::endl
                << "packet loss: " << l << std::endl
                << "packet loss rate: " << (l * 100.0 / seq) << "%"
                << std::endl;
    } else {
      std::cout << "get report error" << std::endl;
    }
  } else {
    std::cout << "server rejected test." << std::endl;
    return;
  }
}

void Client::getBandwidthMsg(char* send_buf, int packetSize) {
  long long timestamp = Util::getTimestamp();
  std::string time_str = std::to_string(timestamp);
  for (int i = 0; i < time_str.length(); i++) {
    send_buf[i] = time_str[i];
  }
  send_buf[time_str.length()] = ',';

  srand(time(0));
  for (int i = time_str.length() + 1; i < packetSize; i++) {
    char c = rand() % 256 - 128;
    send_buf[i] = c;
  }
}

Client::~Client() {
  closesocket(sockfd);
  WSACleanup();
}

cxxopts::ParseResult parse(int argc, char* argv[]) {
  try {
    cxxopts::Options options(argv[0], " - example command line options");

    options.add_options()("r,rtt", "start rtt test", cxxopts::value<bool>())(
        "b,bandwidth", "start bandwidth test",
        cxxopts::value<int>()->default_value("10"))(
        "p,pakcetSize", "set packet size",
        cxxopts::value<int>()->default_value("256"))(
        "t,testSeconds", "test time", cxxopts::value<int>()->default_value("5"),
        "<sec>")("u,unit", "bandwidth unit",
                 cxxopts::value<char>()->default_value("k"));

    auto result = options.parse(argc, argv);

    return result;

  } catch (const cxxopts::OptionException& e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }
}

int main(int argc, char* argv[]) {
  auto result = parse(argc, argv);

  Client client("127.0.0.1", 8080);

  if (result.count("r")) {
    client.rttTest();
  } else {
    int bandwidth = result["b"].as<int>();
    int packetSize = result["p"].as<int>();
    int testSeconds = result["t"].as<int>();
    int bandwidthUnit = result["u"].as<char>();
    client.bandwidthTest(testSeconds, packetSize, bandwidth, bandwidthUnit);
  }

  return 0;
}
