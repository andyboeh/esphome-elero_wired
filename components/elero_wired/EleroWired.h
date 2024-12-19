#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/gpio.h"

#define ELERO_WIRED_RELAY_BLOCK_TIME  500
#define ELERO_WIRED_MAX_RELAY_TIME 60000

namespace esphome {
namespace elero_wired {

class EleroWiredCover;
class EleroWiredSwitch;

typedef enum {
  PENDING_NONE,
  PENDING_SET_BOTH_RELAYS,
  PENDING_CLEAR_BOTH_RELAYS,
} t_pending_requests_;

class EleroWired : public Component {
  void setup() override;
  void loop() override;

 public:
  void set_pin_open(GPIOPin *pin) { this->pin_open_ = pin; }
  void set_pin_close(GPIOPin *pin) { this->pin_close_ = pin; }
  void pin_open_set(void);
  void pin_close_set(void);
  void pin_open_clear(void);
  void pin_close_clear(void);
  void pin_open_close_set(void);
  void pin_open_close_clear(void);
  void register_cover(EleroWiredCover *cover) { this->cover_ = cover; }

 protected:
  GPIOPin *pin_open_{nullptr};
  GPIOPin *pin_close_{nullptr};
  bool pin_open_state_{false};
  bool pin_close_state_{false};
  EleroWiredCover *cover_{nullptr};
  uint32_t last_relay_time_{0};
  t_pending_requests_ pending_requests_{PENDING_NONE};
};

} // namespace elero_wired
} //namespace esphome
