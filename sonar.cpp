#include "sonar.h"
#include "lgpio.h"
#include <iostream>

Sonar::Sonar(uint32_t t, uint32_t r) : trigger_(t), response_(r) {
  io_handle_ = lgGpiochipOpen(4);
  lgGpioClaimOutput(io_handle_, 0, trigger_, 0);
  lgGpioClaimInput(io_handle_, LG_SET_PULL_DOWN, r);
}

  double Sonar::get_distance() {
    ping();
    uint64_t cost_time = pong();

    //std::cout<<"transfer cost time..............:"<<cost_time<<std::endl;
    double distance = cost_time *
                        (343.2 / 1000 / 1000) /*meters per micro second*/ /
                        2 /*go and back,*/;
    return distance;
  }

  void Sonar::ping() {
    uint64_t start = lguTimestamp();
    //trigger signal start
    lgGpioWrite(io_handle_,trigger_,1);
    //trigger 10 us
    lguSleep((1.0/1000/1000)*10);
    //trigger signal end
    lgGpioWrite(io_handle_,trigger_,0);
    uint64_t now = lguTimestamp();
    //std::cout<<" ping cost time:"<<(now-start)/1000<<std::endl;
  }

  uint64_t Sonar::pong() {
    //std::cout<<"start to pong, value:"<<lgGpioRead(io_handle_,response_)<<std::endl;
    uint64_t start = lguTimestamp();
    uint64_t now;
    int value = 0;
    do {
      value = lgGpioRead(io_handle_, response_);
      now = lguTimestamp();
    } while (value == 0 && now -start < timeout_);

    if (value == 0) {
      std::cout<<"should not happen, can't get the begin of echo"<<std::endl;
      return 0;
    }

    start = lguTimestamp();
    do {
      value = lgGpioRead(io_handle_, response_);
      now = lguTimestamp();
    } while (value == 1 && now - start < timeout_);
    return (now - start)/1000;
  }