#pragma once
#include <stdint.h>
enum DIR_CTL_VALUE {
  DIR_CTL_LEFT = 500,         /*0.5ms: 0°*/
  DIR_CTL_LEFT_FRONT = 1000,  /*1.0ms: 45°*/
  DIR_CTL_FRONT = 1500,       /*1.5ms: 90°*/
  DIR_CTL_RIGHT_FRONT = 2000, /*2.0ms: 135°*/
  DIR_CTL_RIGHT = 2500,       /*2.5ms: 180°*/
  DIR_CTL_FULL = 20000        /*20ms : width of pulse*/
};
class SteeringGear {
public:
  SteeringGear(uint32_t gpio);
  ~SteeringGear();

  uint32_t move(DIR_CTL_VALUE dir);
  DIR_CTL_VALUE get_pos() { return current_; }

private:
  DIR_CTL_VALUE current_{DIR_CTL_FULL};
  int32_t io_handle_;
  uint32_t control_;
};