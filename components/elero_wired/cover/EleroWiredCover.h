#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/elero_wired/EleroWired.h"

typedef enum {
  COVER_MOVING,
  COVER_MOVING_WAIT,
  COVER_STOPPED,
  COVER_STOPPED_ENDSTOP,
  TILT_MOVING,
  TILT_MOVING_WAIT,
  TILT_STOPPED
} t_elero_wired_states;

namespace esphome {
namespace elero_wired {

class EleroWiredCover : public cover::Cover, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  
  cover::CoverTraits get_traits() override;

  void set_close_duration(uint32_t dur) { this->close_duration_ = dur; }
  void set_open_duration(uint32_t dur) { this->open_duration_ = dur; }
  void set_tilt_close_duration(uint32_t dur) { this->tilt_close_duration_ = dur; }
  void set_tilt_open_duration(uint32_t dur) { this->tilt_open_duration_ = dur; }
  void set_extra_wait_time(uint32_t dur) { this->extra_wait_time_ = dur; }
  void set_endstop_wait_time(uint32_t dur) { this->endstop_wait_time_ = dur; }
  void set_elero_wired_parent(EleroWired *parent) { this->parent_ = parent; }
  void recompute_position();
  void start_movement(cover::CoverOperation op);
  
 protected:
  void control(const cover::CoverCall &call) override;

  t_elero_wired_states wired_states_ = COVER_STOPPED;
  EleroWired *parent_;
  uint32_t movement_start_{0};
  uint32_t open_duration_{0};
  uint32_t close_duration_{0};
  uint32_t tilt_open_duration_{0};
  uint32_t tilt_close_duration_{0};
  uint32_t extra_wait_time_{0};
  uint32_t endstop_wait_time_{0};
  uint32_t last_publish_{0};
  uint32_t last_recompute_time_{0};
  uint32_t last_relay_time_{0};
  uint32_t movement_time_{0};
  float target_position_{1.0};
  float target_tilt_{1.0};
  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
  cover::CoverOperation pending_operation_{cover::COVER_OPERATION_IDLE};
};

} // namespace elero_wired
} // namespace esphome

