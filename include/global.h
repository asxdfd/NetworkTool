#pragma once

#include <chrono>
#include <vector>
#include <istream>
#include <sstream>

class Util {
 public:
  static long long getTimestamp() {
    std::chrono::time_point<std::chrono::system_clock,
                            std::chrono::milliseconds>
        tp = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch());
    long long timestamp = tmp.count();
    return timestamp;
  }

  static std::vector<std::string> split(const char* str, char c) {
    std::vector<std::string> pieces;
    std::istringstream ss(str);
    std::string piece;

    while (std::getline(ss, piece, c)) {
      pieces.push_back(piece);
    }

    return pieces;
  }

};
