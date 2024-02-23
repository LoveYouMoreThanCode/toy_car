#include "steering.h"
#include "lgpio.h"
#include <cmath>

SteeringGear::SteeringGear(uint32_t gpio):control_(gpio) {
  io_handle_ = lgGpiochipOpen(4);
  lgGpioClaimOutput(io_handle_, 0, control_, 0);
}

SteeringGear::~SteeringGear() {
  lgGpiochipClose(io_handle_);
}

uint32_t SteeringGear::move(DIR_CTL_VALUE dir) {
  uint32_t degree = 0;
  if (dir == current_) {
    return degree;
  }
  if (current_ == DIR_CTL_FULL) {
    degree = 180;
  }else {
    degree = std::abs(dir - current_) / 500 * 45;
  }
  lgTxPulse(io_handle_, control_, (int)dir, DIR_CTL_FULL - dir, 0, 0);
  current_ = dir;
  return degree;
}