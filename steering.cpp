#include "steering.h"
#include "lgpio.h"

SteeringGear::SteeringGear(uint32_t gpio):control_(gpio) {
  io_handle_ = lgGpiochipOpen(4);
  lgGpioClaimOutput(io_handle_, 0, control_, 0);
}

SteeringGear::~SteeringGear() {
  lgGpiochipClose(io_handle_);
}

void SteeringGear::move(DIR_CTL_VALUE dir) {
  lgTxPulse(io_handle_, control_, (int)dir, DIR_CTL_FULL - dir, 0, 0);
}