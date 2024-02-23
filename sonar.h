#pragma once
#include <stdint.h>
class Sonar {
public:
  Sonar(uint32_t t, uint32_t r);
  ~Sonar();
  double get_distance();
private:
  void ping();
  uint64_t pong();
private:
  int32_t io_handle_;
  uint32_t trigger_;
  uint32_t response_;
  uint64_t timeout_{20000000};
};