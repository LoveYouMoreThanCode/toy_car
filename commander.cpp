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
#include "steering.h"
#include <cassert>
#include <unordered_map>
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
    if (!js_.isFound()) {
      return "fallback_terminal";
    }
    JoystickEvent event;
    while (js_.sample(&event)) {
      if (event.isButton()) {
        if (event.number == 2)  {
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
    //printf("x=%d y=%d\n", x_, y_);
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
  Joystick js_;
  std::string path_;
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
    if (v2 == 0 && v3 == 0) {
      return "forward";
    }
    if (v2 == 0) {
      return "left";
    }
    if (v3 == 0) {
      return "right";
    }
    return "backward";
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
  SonarCommander(uint32_t p1, uint32_t p2) : sonar_(p1, p2) {
    for (uint32_t i = 1; i <= 32; i++) {
      std::string dir = i % 2 ? "right" : "left";
      for (uint32_t j = 0; j < i*3; j++) {
        lookup_algo_.push_back(dir);
      }
    }
  }
  ~SonarCommander() {}
  std::string scan_cmd() override {
    double cur_distance = sonar_.get_distance();
    switch(state_) {
      case ADJUST:
        state_ = LOOKUP;
        return "brake";
      case WALK:
        if (cur_distance >= safe_distance_low_) {
          return "forward";
        }
        state_ = LOOKUP;
        lookup_cursor_ = 0;
        return "brake";
      case LOOKUP:
        if (cur_distance > safe_distance_high_) {
          state_ = WALK;
          return "brake";
        }
        state_ = LOOKUP;
        return lookup_algo_[lookup_cursor_++ % lookup_algo_.size()];
    }
  }

private:
  enum STATE {
    WALK = 1,
    LOOKUP = 2,
    ADJUST = 3,
  };
  Sonar sonar_;
  STATE state_{LOOKUP};
  double safe_distance_low_{0.5};
  double safe_distance_high_{0.8};
  uint32_t lookup_cursor_{0};
  std::vector<std::string> lookup_algo_;
};
class SteeringSonarCommander : public Commander {
public:
  SteeringSonarCommander(uint32_t sonar_t, uint32_t sonar_r,
                         uint32_t steering_c)
      : sonar_(sonar_t, sonar_r), steering_(steering_c) {
    walk_scan_ = {DIR_CTL_LEFT_FRONT, 
                  DIR_CTL_FRONT, 
                  DIR_CTL_RIGHT_FRONT,
                  DIR_CTL_FRONT};
    stop_scan_ = {DIR_CTL_LEFT,       
                  DIR_CTL_LEFT_FRONT, 
                  DIR_CTL_FRONT,
                  DIR_CTL_RIGHT_FRONT, 
                  DIR_CTL_RIGHT,      
                  DIR_CTL_RIGHT_FRONT,
                  DIR_CTL_FRONT,
                  DIR_CTL_LEFT_FRONT};
  };
  ~SteeringSonarCommander() {}
  std::string scan_cmd() override{
    scan_distance();
    switch(state_) {
      case ADJUST:
        state_ = LOOKUP;
        return "brake";
      case WALK:
        double min_distance = std::min(dir_distance_[DIR_CTL_LEFT_FRONT],
                                       dir_distance_[DIR_CTL_FRONT]);
        min_distance = std::min(dir_distance_[DIR_CTL_RIGHT_FRONT],min_distance);
        if (min_distance >= safe_distance_low_) {
          return "forward";
        }
        state_ = LOOKUP;
        return "brake";
      case LOOKUP:
        double min_distance = std::min(dir_distance_[DIR_CTL_LEFT_FRONT],
                                       dir_distance_[DIR_CTL_FRONT]);
        min_distance = std::min(dir_distance_[DIR_CTL_RIGHT_FRONT],min_distance);
        if (min_distance >= safe_distance_low_) {
          state_ = WALK;
          return "brake";
        }
        state_ = ADJUST;
        if (dir_distance_[DIR_CTL_LEFT] > dir_distance_[DIR_CTL_RIGHT]) {
          return "left";
        }
        return "right";

    }
    if (state_ == ADJUST) {
      state_ = LOOKUP;
      return "brake";
    }
  }
private:
  void scan_distance(){
    if (state_ == ADJUST) {
      return;
    }
    auto *scan_road = state_ == LOOKUP ? &stop_scan_ : &walk_scan_;
    uint32_t scan_cnt = state_ == LOOKUP ? 5 : 3;
    for (uint32_t i = 0; i < scan_cnt; i++) {
      auto steering_dir = (*scan_road)[scan_cursor_%(scan_road->size())];
      steering_.move(steering_dir);
      //空载速度: 0.13秒/60°  --> 130000 us/60°
      //按照规划的路径扫描。首次可以不用移动，第2和第3次扫描行走扫描需要扫描45°,stop扫描需要90°
      double need_wait_s = 0.13 / 60 * (state_ == LOOKUP ? 90 : 45);
      lguSleep(need_wait_s);
      dir_distance_[steering_dir] = sonar_.get_distance();
      scan_cursor_ ++;
    }
  }
private:
  enum STATE {
    WALK = 1,
    LOOKUP = 2,
    ADJUST = 3,
  };
  double safe_distance_low_{0.4};
  double safe_distance_high_{0.7};
  Sonar sonar_;
  /*
  * determine if continue to go
  */
  std::vector<DIR_CTL_VALUE> walk_scan_;

  /*determine direction for next*/
  std::vector<DIR_CTL_VALUE> stop_scan_;

  uint32_t scan_cursor_{0};
  //save distance value
  std::unordered_map<DIR_CTL_VALUE,double> dir_distance_;

  SteeringGear steering_;

  STATE state_{LOOKUP};
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
