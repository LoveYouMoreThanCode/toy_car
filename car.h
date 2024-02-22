
extern "C" {
#include "lgpio.h"
}
#include "commander.h"
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <memory>
#include <unordered_map>

#define MOTOR_DRIVE_PWM_FREQ_HZ 1000 /*Hz*/
class Motor {
public:
  Motor(int controller, const std::string &name, uint32_t p1, uint32_t p2)
      : ctl_(controller), name_(name), p1_(p1), p2_(p2) {}
  Motor() : ctl_(-1), name_("unkown"), p1_(UINT32_MAX), p2_(UINT32_MAX) {}
  ~Motor() {}
  int init();
  void move_forward(uint32_t speed);
  void move_backward(uint32_t speed);
  void brake();

private:
  uint32_t revise_speed(uint32_t speed);

private:
  int ctl_;
  std::string name_;
  uint32_t p1_;
  uint32_t p2_;
};

class Car {
public:
  Car(){};
  ~Car(){};
  int init();
  void move_forward();
  void move_backward();
  void turn_left(bool spin = true);
  void turn_right(bool spin = true);
  void brake();
  void set_engine(uint32_t f_speed, uint32_t b_speed, uint32_t t_speed);

private:
  int32_t ctl_handle_;
  std::unordered_map<std::string, Motor> motors_;
  uint32_t forward_speed_{90};
  uint32_t backward_speed_{50};
  uint32_t turn_speed_{30};
};
