#include "car.h"

int Motor::init() {
  // AT8236驱动方式：IN1=1 IN2=1 --> 刹车
  // 默认电平设置为1
  int rc1 = lgGpioClaimOutput(ctl_, 0, p1_, 1);
  int rc2 = lgGpioClaimOutput(ctl_, 0, p2_, 1);
  if (rc1 || rc2) {
    return -1;
  }
  return 0;
}

void Motor::move_forward(uint32_t speed) {
  std::cout << "motor:" << name_ << " move forward,"
            << " ctl:" << ctl_ << " p1:" << p1_ << " p2:" << p2_
            << " speed:" << speed << std::endl;
  // AT8236驱动方式：IN1=1 IN2=0 --> 正转
  // lgGpioWrite(ctl_, p1_, 1);
  // lgGpioWrite(ctl_, p2_, 0);
  lgTxPwm(ctl_, p1_, MOTOR_DRIVE_PWM_FREQ_HZ, revise_speed(speed), 0, 0);
  lgTxPwm(ctl_, p2_, 0, 0, 0);
}

void Motor::move_backward(uint32_t speed) {
  std::cout << "motor:" << name_ << " move backward,"
            << " ctl:" << ctl_ << " p1:" << p1_ << " p2:" << p2_
            << " speed:" << speed << std::endl;
  // AT8236驱动方式：IN1=0 IN2=1 --> 反转
  // lgGpioWrite(ctl_, p1_, 0);
  // lgGpioWrite(ctl_, p2_, 1);
  lgTxPwm(ctl_, p1_, 0, 0, 0);
  lgTxPwm(ctl_, p2_, MOTOR_DRIVE_PWM_FREQ_HZ, revise_speed(speed), 0, 0);
}

void Motor::brake() {
  std::cout << "motor:" << name_ << " brake,"
            << " ctl:" << ctl_ << " p1:" << p1_ << " p2:" << p2_ << std::endl;
  // AT8236驱动方式：IN1=1 IN2=1 --> 刹车
  // lgGpioWrite(ctl_, p1_, 0);
  // lgGpioWrite(ctl_, p2_, 0);
  lgTxPwm(ctl_, p1_, 0, 0, 0);
  lgTxPwm(ctl_, p2_, 0, 0, 0);
}

uint32_t Motor::revise_speed(uint32_t speed) {
  return std::min(100, std::max(20, speed));
}

int Car::init() {
  ctl_handle_ = lgGpiochipOpen(4);
  assert(ctl_handle_ >= 0);
  uint32_t left_motor_p1 = 17;
  uint32_t left_motor_p2 = 27;
  Motor left(ctl_handle_, "left_rear", left_motor_p1, left_motor_p2);
  int rc = left.init();
  if (rc) {
    return -1;
  }
  uint32_t right_motor_p1 = 23;
  uint32_t right_motor_p2 = 24;
  Motor right(ctl_handle_, "right_rear", right_motor_p1, right_motor_p2);
  rc = right.init();
  if (rc) {
    return rc;
  }
  motors_["left_rear"] = left;
  motors_["right_rear"] = right;

  // add two mor motors here
  uint32_t left_front_p1 = 5;
  uint32_t left_front_p2 = 6;
  Motor left1(ctl_handle_, "left_front", left_front_p1, left_front_p2);
  rc = left1.init();
  if (rc) {
    return -1;
  }
  uint32_t right_front_p1 = 20;
  uint32_t right_front_p2 = 21;
  Motor right1(ctl_handle_, "right_front", right_front_p1, right_front_p2);
  rc = right1.init();
  if (rc) {
    return rc;
  }
  motors_["left_front"] = left1;
  motors_["right_front"] = right1;

  return 0;
}

void Car::move_forward() {
  motors_["left_rear"].move_forward(forward_speed_);
  motors_["right_rear"].move_forward(forward_speed_);
  motors_["left_front"].move_forward(forward_speed_);
  motors_["right_front"].move_forward(forward_speed_);
}

void Car::move_backward() {
  motors_["left_rear"].move_backward(backward_speed_);
  motors_["right_rear"].move_backward(backward_speed_);
  motors_["left_front"].move_backward(backward_speed_);
  motors_["right_front"].move_backward(backward_speed_);
}

void Car::turn_left(bool spin) {
  motors_["left_rear"].move_forward(turn_speed_);
  motors_["left_front"].move_forward(turn_speed_);
  if (spin) {
    motors_["right_rear"].move_backward(turn_speed_);
    motors_["right_front"].move_backward(turn_speed_);
  } else {
    motors_["right_rear"].brake();
    motors_["right_front"].brake();
  }
}

void Car::turn_right(bool spin) {
  motors_["right_rear"].move_forward(turn_speed_);
  motors_["right_front"].move_forward(turn_speed_);
  if (spin) {
    motors_["left_rear"].move_backward(turn_speed_);
    motors_["left_front"].move_backward(turn_speed_);
  } else {
    motors_["left_rear"].brake();
    motors_["left_front"].brake();
  }
}

void Car::brake() {
  motors_["left_rear"].brake();
  motors_["right_rear"].brake();
  motors_["left_front"].brake();
  motors_["right_front"].brake();
}

void Car::set_engine(uint32_t f_speed, uint32_t b_speed, uint32_t t_speed) {
  forward_speed_ = f_speed;
  backward_speed_ = b_speed;
  turn_speed_ = t_speed;
}
