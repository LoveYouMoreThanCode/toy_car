#include <iostream>
#include <unistd.h>

#ifdef USE_REAL_GPIO
extern "C" {
#include "lgpio.h"
}
#else
int lgGpioClaimOutput(int handle, int lFlags, int gpio, int level) {
  std::cout << "claim gpio:" << gpio << " as output with level:" << level
            << std::endl;
  return 0;
}
int lgGpioWrite(int handle, int gpio, int level) {
  std::cout << "set gpio:" << gpio << " to level:" << level << std::endl;
  return 0;
}
int lgGpiochipOpen(int gpioDev) {
  int handle_fd = 100;
  std::cout << "open gpioDEV:" << gpioDev << " handle_fd:" << handle_fd
            << std::endl;
  return handle_fd;
}
void lguSleep(double sleepSecs) {
  usleep(sleepSecs*1000UL*1000UL);
  std::cout << "sleep..., time:" << (sleepSecs * 1000UL) << " ms" << std::endl;
}
#endif
#include <cassert>
#include <unordered_map>


class Motor {
public:
  Motor(int controller, const std::string &name, uint32_t p1, uint32_t p2)
      : ctl_(controller), name_(name), p1_(p1), p2_(p2) {}
  Motor() : ctl_(-1), name_("unkown"), p1_(UINT32_MAX), p2_(UINT32_MAX) {}
  ~Motor() {}
  int init() {
    // AT8236驱动方式：IN1=1 IN2=1 --> 刹车
    // 默认电平设置为1
    int rc1 = lgGpioClaimOutput(ctl_, 0, p1_, 1);
    int rc2 = lgGpioClaimOutput(ctl_, 0, p2_, 1);
    if (rc1 || rc2) {
      return -1;
    }
    return 0;
  }
  void move_forward() {
    std::cout << "motor:" << name_ << " move forward" << std::endl;
    // AT8236驱动方式：IN1=1 IN2=0 --> 正转
    lgGpioWrite(ctl_, p1_, 1);
    lgGpioWrite(ctl_, p2_, 0);
  }
  void move_backward() {
    std::cout << "motor:" << name_ << " move backward" << std::endl;
    // AT8236驱动方式：IN1=0 IN2=1 --> 反转
    lgGpioWrite(ctl_, p1_, 0);
    lgGpioWrite(ctl_, p2_, 1);
  }
  void brake() {
    std::cout << "motor:" << name_ << " brake" << std::endl;
    // AT8236驱动方式：IN1=1 IN2=1 --> 反转
    lgGpioWrite(ctl_, p1_, 1);
    lgGpioWrite(ctl_, p2_, 1);
  }

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
  int init() {
    ctl_handle_ = lgGpiochipOpen(4);
    assert(ctl_handle_ != 0);
    uint32_t left_motor_p1 = 17;
    uint32_t left_motor_p2 = 27;
    Motor left(ctl_handle_, "left", left_motor_p1, left_motor_p2);
    int rc = left.init();
    if (rc) {
      return -1;
    }
    uint32_t right_motor_p1 = 23;
    uint32_t right_motor_p2 = 24;
    Motor right(ctl_handle_, "right", right_motor_p1, right_motor_p2);
    rc = right.init();
    if (rc) {
      return rc;
    }
    motors_["left"] = left;
    motors_["right"] = right;
    return 0;
  }
  void move_forward() {
    motors_["left"].move_forward();
    motors_["right"].move_forward();
  }
  void move_backward() {
    motors_["left"].move_backward();
    motors_["right"].move_backward();
  }
  void turn_left(bool spin = false) {
    motors_["left"].move_forward();
    if (spin) {
      motors_["right"].move_backward();
    } else {
      motors_["right"].brake();
    }
  }
  void turn_right(bool spin = false) {
    motors_["right"].move_forward();
    if (spin) {
      motors_["left"].move_backward();
    } else {
      motors_["left"].brake();
    }
  }
  void brake() {
    motors_["left"].brake();
    motors_["right"].brake();
  }

private:
  int32_t ctl_handle_;
  std::unordered_map<std::string, Motor> motors_;
};


int main() {
  Car my_car;
  int rc = my_car.init();
  if (rc) {
    std::cout<<"failed to init my car, rc:"<<rc<<std::endl;
    return rc;
  }
  //前进200ms
  std::cout<<"前进200ms"<<std::endl;
  my_car.move_forward();
  lguSleep(0.2);
  //后退200ms
  std::cout<<"后退200ms"<<std::endl;
  my_car.move_backward();
  lguSleep(0.2);
  //spin方式左转100ms
  std::cout<<"原地左转100ms"<<std::endl;
  my_car.turn_left(true);
  lguSleep(0.1);
  //spin方式右转100ms
  std::cout<<"原地右转100ms"<<std::endl;
  my_car.turn_right(true);
  lguSleep(0.1);
  //普通左转100ms 
  std::cout<<"普通左转100ms"<<std::endl;
  my_car.turn_left();
  lguSleep(0.1);
  //普通右转100ms
  std::cout<<"普通右转100ms"<<std::endl;
  my_car.turn_right();
  lguSleep(0.1);

  while (true) {
    std::string cmd;
    std::cin >> cmd;
    if (cmd == "left") {
      std::cout << "普通左转100ms............" << std::endl;
      my_car.turn_left();
      lguSleep(0.1);
    } else if (cmd == "right") {
      std::cout << "普通右转100ms............" << std::endl;
      my_car.turn_right();
      lguSleep(0.1);
    } else if (cmd == "forward") {
      std::cout << "前进200ms................" << std::endl;
      my_car.move_forward();
      lguSleep(0.2);
    } else if (cmd == "backward") {
      std::cout << "后退200ms................" << std::endl;
      my_car.move_backward();
      lguSleep(0.2);
    } else {
      std::cout << "崩溃200ms..............." << std::endl;
      std::cout << "原地左转100ms" << std::endl;
      my_car.turn_left(true);
      lguSleep(0.1);
      std::cout << "原地右转100ms" << std::endl;
      my_car.turn_right(true);
      lguSleep(0.1);
    }
  }
  //结束
  return 0;
}