#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/elero_wired/EleroWired.h"

namespace esphome {
namespace elero_wired {

class EleroWiredSwitch : public switch_::Switch, public Component {
 public:
  void setup() override;

  void set_elero_wired_parent(EleroWired *parent) { this->parent_ = parent; }

 protected:
  void write_state(bool state) override;

  EleroWired *parent_;

};

}  // namespace elero_wired
}  // namespace esphome


