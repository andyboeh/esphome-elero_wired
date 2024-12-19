#include "EleroWiredSwitch.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace elero_wired {

static const char *const TAG = "elero_wired.switch";

void EleroWiredSwitch::setup() {
    bool initial_state = this->get_initial_state().value_or(false);
    if (initial_state) {
        this->turn_on();
    } else {
        this->turn_off();
    }
}

void EleroWiredSwitch::write_state(bool state) {
    if(state)
      this->parent_->pin_open_close_set();
    else
      this->parent_->pin_open_close_clear();
    this->publish_state(state);
}

}  // namespace elero_wired
}  // namespace esphome

