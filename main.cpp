
#include "commander.h"
#include <iostream>
#include <unistd.h>


extern "C" {
#include "lgpio.h"
}
#include <cassert>
#include <memory>
#include <unordered_map>
#include "car.h"


int main() {
  Car my_car;
  int rc = my_car.init();
  if (rc) {
    std::cout << "failed to init my car, rc:" << rc << std::endl;
    return rc;
  }
#if 0
  std::unique_ptr<Commander, void (*)(Commander *)> commander(
      make_commander("joystick"), destroy_commander);
  my_car.set_engine(90, 50, 30);
#else
  std::unique_ptr<Commander, void (*)(Commander *)> commander(
      make_commander("infrared"), destroy_commander);
  my_car.set_engine(30, 15, 5);
#endif
  while (true) {
    // 保持一定的控制周期
    lguSleep(0.1);

    // 命令输入提示
    std::cout << std::endl << std::endl;
    std::cout << "...........等待输入指令left(l)/right(r)/forward(f)/"
                 "backward(b)/brake(*)....."
              << std::endl;
    std::string cmd = commander->scan_cmd();

    if (cmd == "left" || cmd == "l") {
      std::cout << "............普通左转100ms............" << std::endl;
      my_car.turn_left();
    } else if (cmd == "right" || cmd == "r") {
      std::cout << "............普通右转100ms............" << std::endl;
      my_car.turn_right();
    } else if (cmd == "forward" || cmd == "f") {
      std::cout << "............前进100ms................" << std::endl;
      my_car.move_forward();
    } else if (cmd == "backward" || cmd == "b") {
      std::cout << "............后退100ms................" << std::endl;
      my_car.move_backward();
    } else {
      std::cout << "............刹车..............." << std::endl;
      my_car.brake();
    }
  }
  // 结束
  return 0;
}
