// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright Drew Noakes 2013-2016
#include "commander.h"
#include "joystick.h"
#include "sonar.h"
#include <cassert>
#include <vector>
extern "C" {
#include "lgpio.h"
}
class JsCommander : public Commander {
public:
  JsCommander(std::string path) : js_(path), path_(path) {}
  ~JsCommander() {}
  std::string scan_cmd() override {
    reload_if_need();
    JoystickEvent event;
    while (js_.sample(&event)) {
      if (event.isButton()) {
        if (event.number == 7)  {
          sonar_on_ = event.value;
        }
        continue;
      }
      if (!event.isAxis()) {
        continue;
      }
      if (event.number == 4) {
        x_ = event.value;
      } else if (event.number == 5) {
        y_ = event.value;
      }
    }
    printf("x=%d y=%d\n", x_, y_);
    return make_cmd();
  }

private:
  std::string make_cmd() {
    if (sonar_on_) {
      return "auto_sonar";
    }
    if (x_ == 0 && y_ == 0) {
      return "brake";
    }
    if (x_ < 0) {
      return "forward";
    }
    if (y_ == 0) {
      return "backward";
    } else if (y_ < 0) {
      return "right";
    } else {
      return "left";
    }
  }
  void reload_if_need() {
    if (js_.isFound()) {
      return;
    }
    js_.~Joystick();
    new (&js_) Joystick(path_);
  }

private:
  int x_{0};
  int y_{0};
  bool sonar_on_{false};
  std::string path_;
  Joystick js_;
};

class TerminalCommander : public Commander {
public:
  TerminalCommander() {}
  ~TerminalCommander() {}
  std::string scan_cmd() override {
    std::string cmd;
    std::cin >> cmd;
    return cmd;
  }
};

class InfraredCommander : public Commander {
public:
  InfraredCommander(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
      : p1_(p1), p2_(p2), p3_(p3), p4_(p4) {
    io_handle_ = lgGpiochipOpen(4);
    set_all_port_input();
  }
  ~InfraredCommander() {}
  std::string scan_cmd() override {
    int v1 = lgGpioRead(io_handle_, p1_);
    int v2 = lgGpioRead(io_handle_, p2_);
    int v3 = lgGpioRead(io_handle_, p3_);
    int v4 = lgGpioRead(io_handle_, p4_);
    return make_cmd(v1, v2, v3, v4);
  }

private:
  std::string make_cmd(int32_t v1, int32_t v2, int32_t v3, int32_t v4) {
    if (v4 == 0) {
      return "right";
    }
    if (v1 == 0) {
      return "left";
    }
    if (v2 == 0 & v3 == 0) {
      return "forward";
    }
    if (v2 == 0) {
      return "left";
    }
    if (v3 == 0) {
      return "right";
    }
    return "backward"
  }
  void set_all_port_input() {
    lgGpioClaimInput(io_handle_, LG_SET_PULL_UP, p1_);
    lgGpioClaimInput(io_handle_, LG_SET_PULL_UP, p2_);
    lgGpioClaimInput(io_handle_, LG_SET_PULL_UP, p3_);
    lgGpioClaimInput(io_handle_, LG_SET_PULL_UP, p4_);
  }

private:
  int32_t io_handle_;
  uint32_t p1_;
  uint32_t p2_;
  uint32_t p3_;
  uint32_t p4_;
};

class SonarCommander : public Commander {
public:
  SonarCommander(uint32_t p1, SonarCommander p2) : sonar_(p1, p2) {
    for (uint32_t i = 1; i <= 32; i++) {
      std::string dir = i % 2 ? "right" : "left";
      for (uint32_t j = 0; j < i; j++) {
        lookup_algo_.push_back(dir);
      }
    }
  }
  ~SonarCommander() {}
  std::string scan_cmd() override {
    double cur_distance = sonar_.get_distance();
    if (cur_distance > safe_distance_) {
      state_ = WALK;
    } else {
      if (state_ != LOOKUP) {
        state_ = LOOKUP;
        lookup_cursor_ = 0;
      }
    }
    if (state_ == WALK) {
      return "forward";
    }
    return lookup_algo_[lookup_cursor_++ % lookup_algo_.size()];
  }

private:
  enum STATE {
    WALK = 1,
    LOOKUP = 2,
  };
  Sonar sonar_;
  STATE state_{WALK};
  double safe_distance_{0.5};
  uint32_t lookup_cursor_{0};
  std::vector<std::string> lookup_algo_;
};

Commander *make_commander(std::string type) {
  if (type == "joystick") {
    return new JsCommander("/dev/input/js0");
  } else if (type == "terminal") {
    return new TerminalCommander();
  } else if (type == "infrared") {
    return new InfraredCommander(25, 8, 7, 1);
  } else if (type == "sonar") {
    return new SonarCommander(14, 15);
  }
  return nullptr;
}
void destroy_commander(Commander *cmd) { delete cmd; }