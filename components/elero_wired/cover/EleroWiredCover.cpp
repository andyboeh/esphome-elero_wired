#include "EleroWiredCover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace elero_wired {

using namespace esphome::cover;

static const char *const TAG = "elero_wired.cover";

void EleroWiredCover::dump_config() {
  LOG_COVER("", "EleroWiredCover", this);
}

void EleroWiredCover::setup() {
  this->parent_->register_cover(this);
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
    // If the position is not completely closed,
    // the tilt has to be force open
    if(this->position > COVER_CLOSED)
      this->tilt = COVER_OPEN;
  } else {
    if((this->open_duration_ > 0) && (this->close_duration_ > 0))
      this->position = 0.5f;
      this->tilt = COVER_OPEN;
  }
}

void EleroWiredCover::loop() {
  uint32_t now = millis();

  if((this->current_operation != COVER_OPERATION_IDLE)) {
    if((this->open_duration_ > 0) && (this->close_duration_ > 0)) {
      this->recompute_position();
    } else {
      if((now - this->movement_start_) > ELERO_WIRED_MAX_RELAY_TIME) {
        if(this->current_operation == COVER_OPERATION_OPENING) {
          this->position = COVER_OPEN;
          this->parent_->pin_open_clear();
        } else {
          this->position = COVER_CLOSED;
          this->parent_->pin_close_clear();
        }
        this->current_operation = COVER_OPERATION_IDLE;
        this->last_relay_time_ = now;
        this->publish_state();
      }
    }

    // Publish position every second
    if(now - this->last_publish_ > 1000) {
      this->publish_state(false);
      this->last_publish_ = now;
    }
  } else {
    if((this->wired_states_ == COVER_STOPPED_ENDSTOP) && ((now - this->movement_time_) > this->endstop_wait_time_)) {
      this->wired_states_ = COVER_STOPPED;
      this->parent_->pin_open_clear();
      this->parent_->pin_close_clear();
      this->last_relay_time_ = now;
    }
  }

  if((this->pending_operation_ != COVER_OPERATION_IDLE) && ((now - this->last_relay_time_) > ELERO_WIRED_RELAY_BLOCK_TIME)) {
    ESP_LOGD(TAG, "loop running pending operation");
    this->start_movement(this->pending_operation_);
  }
}

float EleroWiredCover::get_setup_priority() const { return setup_priority::DATA; }

cover::CoverTraits EleroWiredCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(true);
  if((this->open_duration_ > 0) && (this->close_duration_ > 0)) {
    traits.set_supports_position(true);
    traits.set_is_assumed_state(false);
  } else {
    traits.set_supports_position(false);
    traits.set_is_assumed_state(true);
  }
  traits.set_supports_toggle(true);

  if((this->tilt_open_duration_ > 0) && (this->tilt_close_duration_ > 0))
    traits.set_supports_tilt(true);
  else
    traits.set_supports_tilt(false);
  return traits;
}

void EleroWiredCover::control(const cover::CoverCall &call) {
  uint32_t now = millis();

  if(this->status_has_warning()) {
    ESP_LOGD(TAG, "Controls disabled due to wireless mode");
    return;
  }

  if(this->wired_states_ == COVER_STOPPED_ENDSTOP) {
    this->parent_->pin_open_clear();
    this->parent_->pin_close_clear();
    this->last_relay_time_ = now;
    this->wired_states_ = COVER_STOPPED;
  }

  if (call.get_stop()) {
    ESP_LOGD(TAG, "call.get_stop()");
    this->target_position_ = this->position;
    this->target_tilt_ = this->tilt;
    this->start_movement(COVER_OPERATION_IDLE);
  }
  if (call.get_position().has_value()) {
    ESP_LOGD(TAG, "call.get_position()");
    auto pos = *call.get_position();
    this->target_position_ = pos;
    if((this->open_duration_ == 0) || (this->close_duration_ == 0)) {
      if(this->target_position_ == COVER_OPEN) {
        this->start_movement(COVER_OPERATION_OPENING);
      } else {
        this->start_movement(COVER_OPERATION_CLOSING);
      }
      return;
    }
    // If we are currently tilted, we need to undo first.
    if(this->tilt != COVER_OPEN) {
      this->target_tilt_ = COVER_OPEN;
      this->wired_states_ = TILT_MOVING;
      this->start_movement(COVER_OPERATION_OPENING);
    } else {
      if(this->target_position_ > this->position) {
        this->wired_states_ = COVER_MOVING;
        this->start_movement(COVER_OPERATION_OPENING);
      } else if(this->target_position_ < this->position) {
        this->wired_states_ = COVER_MOVING;
        this->start_movement(COVER_OPERATION_CLOSING);
      }
    }
  }
  if (call.get_tilt().has_value()) {
    ESP_LOGD(TAG, "call.get_tilt()");
    auto tlt = *call.get_tilt();
    this->target_tilt_ = tlt;
    if(this->position != COVER_CLOSED) {
      this->target_position_ = COVER_CLOSED;
      this->start_movement(COVER_OPERATION_CLOSING);
      this->wired_states_ = COVER_MOVING;
    } else {
      if(this->target_tilt_ > this->tilt) {
        this->wired_states_ = TILT_MOVING;
        this->start_movement(COVER_OPERATION_OPENING);
      } else if(this->target_tilt_ < this->tilt) {
        this->wired_states_ = TILT_MOVING;
        this->start_movement(COVER_OPERATION_CLOSING);
      }
    }
  }

  if (call.get_toggle().has_value()) {
    ESP_LOGD(TAG, "call.get_toggle()");

    if(this->current_operation != COVER_OPERATION_IDLE) {
      this->start_movement(COVER_OPERATION_IDLE);
    } else {
      if(this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_OPEN;
        if(this->tilt != COVER_OPEN) {
          this->target_tilt_ = COVER_OPEN;
          this->wired_states_ = TILT_MOVING;
        } else {
          this->wired_states_ = COVER_MOVING;
        }
        this->start_movement(COVER_OPERATION_OPENING);
      } else {
        this->target_position_ = COVER_CLOSED;
        if(this->tilt != COVER_OPEN) {
          this->target_tilt_ = COVER_OPEN;
          this->wired_states_ = TILT_MOVING;
          this->start_movement(COVER_OPERATION_OPENING);
        } else {
          this->wired_states_ = COVER_MOVING;
          this->start_movement(COVER_OPERATION_CLOSING);
        }
      }
    }
  }
}

void EleroWiredCover::start_movement(cover::CoverOperation op) {
  if(op == this->current_operation)
    return;
  
  uint32_t now = millis();

  switch(op) {
  case COVER_OPERATION_OPENING:
    // Stop movement if we are currently closing
    if(this->last_operation_ == COVER_OPERATION_CLOSING) {
      this->parent_->pin_open_clear();
      this->parent_->pin_close_clear();
      this->pending_operation_ = COVER_OPERATION_OPENING;
      this->last_relay_time_ = now;
      this->last_operation_ = COVER_OPERATION_IDLE;
    }
    if((now - this->last_relay_time_) > ELERO_WIRED_RELAY_BLOCK_TIME) {
      this->parent_->pin_open_set();
      this->pending_operation_ = COVER_OPERATION_IDLE;
      this->last_operation_ = COVER_OPERATION_OPENING;
      this->last_relay_time_ = now;
      this->movement_start_ = now;
    } else {
      this->pending_operation_ = COVER_OPERATION_OPENING;
    }
  break;
  case COVER_OPERATION_CLOSING:
    // Stop movement if we are currently opening
    if(this->last_operation_ == COVER_OPERATION_OPENING) {
      this->parent_->pin_open_clear();
      this->parent_->pin_close_clear();
      this->pending_operation_ = COVER_OPERATION_CLOSING;
      this->last_operation_ = COVER_OPERATION_IDLE;
      this->last_relay_time_ = now;
    }
    if((now - this->last_relay_time_) > ELERO_WIRED_RELAY_BLOCK_TIME) {
      this->parent_->pin_close_set();
      this->pending_operation_ = COVER_OPERATION_IDLE;
      this->last_operation_ = COVER_OPERATION_CLOSING;
      this->last_relay_time_ = now;
      this->movement_start_ = now;
    } else {
      this->pending_operation_ = COVER_OPERATION_CLOSING;
    }
  break;

  case COVER_OPERATION_IDLE:
  default:
    this->parent_->pin_open_clear();
    this->parent_->pin_close_clear();
    this->pending_operation_ = COVER_OPERATION_IDLE;
    this->last_relay_time_ = now;
    this->last_operation_ = COVER_OPERATION_IDLE;
    break;
  }

  if(pending_operation_ == COVER_OPERATION_IDLE) {
    this->current_operation = op;
    this->last_recompute_time_ = now;
    this->publish_state();
  }
}

void EleroWiredCover::recompute_position() {
  if(this->current_operation == COVER_OPERATION_IDLE)
    return;
  
  uint32_t now = millis();

  if(this->current_operation == COVER_OPERATION_OPENING) {
    switch(this->wired_states_) {
    case TILT_MOVING:
      if(this->tilt >= this->target_tilt_) { // We reached the tilt
        this->movement_time_ = now;
        if(this->target_tilt_ == COVER_OPEN) {
          if(this->position >= this->target_position_) { // We reached the target position as well
            this->wired_states_ = COVER_STOPPED_ENDSTOP;
          } else {
            this->wired_states_ = TILT_MOVING_WAIT;
          }
        } else {
          this->wired_states_ = TILT_STOPPED;
        }
      } else {
        this->tilt += 1.0f * (now - this->last_recompute_time_) / this->tilt_open_duration_;
        this->tilt = clamp(this->tilt, 0.0f, 1.0f);
      }
    break;
    case TILT_MOVING_WAIT:
      if((now - this->extra_wait_time_) > this->movement_time_) {
        this->wired_states_ = TILT_STOPPED;
        this->movement_time_ = now;
      }
    break;
    case TILT_STOPPED:
      this->parent_->pin_open_clear();
      if(this->position >= this->target_position_) {
        this->wired_states_ = COVER_STOPPED;
      } else if((now - ELERO_WIRED_RELAY_BLOCK_TIME) > this->movement_time_) {
        this->parent_->pin_open_set();
        this->wired_states_ = COVER_MOVING;
        this->movement_time_ = now;
      }
    break;
    case COVER_MOVING:
      if(this->position >= this->target_position_) {
        this->movement_time_ = now;
        if(this->target_position_ == COVER_OPEN) {
          this->wired_states_ = COVER_STOPPED_ENDSTOP;
        } else {
          this->wired_states_ = COVER_STOPPED;
        }
      } else {
        this->position += 1.0f * (now - this->last_recompute_time_) / this->open_duration_;
        this->position = clamp(this->position, 0.0f, 1.0f);
      }
    break;
    case COVER_MOVING_WAIT:
      if((now - this->extra_wait_time_) > this->movement_time_) {
        this->wired_states_ = COVER_STOPPED;
        this->movement_time_ = now;
      }
    break;
    case COVER_STOPPED_ENDSTOP:
      this->current_operation = COVER_OPERATION_IDLE;
      this->publish_state();
    break;
    case COVER_STOPPED:
      this->parent_->pin_open_clear();
      this->current_operation = COVER_OPERATION_IDLE;
      this->publish_state();
    break;
    }
  } else if(this->current_operation == COVER_OPERATION_CLOSING) {
    switch(this->wired_states_) {
    case TILT_MOVING:
      if(this->tilt <= this->target_tilt_) { // We reached the tilt
        this->movement_time_ = now;
        if(this->target_tilt_ == COVER_CLOSED) {
          this->wired_states_ = COVER_STOPPED_ENDSTOP;
        } else {
          this->wired_states_ = TILT_STOPPED;
        }
      } else {
        this->tilt += -1.0f * (now - this->last_recompute_time_) / this->tilt_close_duration_;
        this->tilt = clamp(this->tilt, 0.0f, 1.0f);
      }
    break;
    case TILT_MOVING_WAIT:
      if((now - this->extra_wait_time_) > this->movement_time_) {
        this->wired_states_ = TILT_STOPPED;
        this->movement_time_ = now;
      }
    break;
    case TILT_STOPPED:
      this->parent_->pin_close_clear();
      this->current_operation = COVER_OPERATION_IDLE;
      this->publish_state();
    break;
    case COVER_MOVING:
      if(this->position <= this->target_position_) {
        this->movement_time_ = now;
        if(this->target_position_ == COVER_CLOSED) {
          if(this->tilt <= this->target_tilt_) {
            this->wired_states_ = COVER_STOPPED_ENDSTOP;
          } else {
            this->wired_states_ = COVER_MOVING_WAIT;
          }
        } else {
          this->wired_states_ = COVER_STOPPED;
        }
      } else {
        this->position += -1.0f * (now - this->last_recompute_time_) / this->close_duration_;
        this->position = clamp(this->position, 0.0f, 1.0f);
      }
    break;
    case COVER_MOVING_WAIT:
      if((now - this->extra_wait_time_) > this->movement_time_) {
        this->wired_states_ = COVER_STOPPED;
        this->movement_time_ = now;
      }
    break;
    case COVER_STOPPED_ENDSTOP:
      this->current_operation = COVER_OPERATION_IDLE;
      this->publish_state();
    break;
    case COVER_STOPPED:
      this->parent_->pin_close_clear();
      if(this->tilt <= this->target_tilt_) {
        this->wired_states_ = TILT_STOPPED;
      } else if((now - ELERO_WIRED_RELAY_BLOCK_TIME) > this->movement_time_) {
        this->parent_->pin_close_set();
        this->wired_states_ = TILT_MOVING;
        this->movement_time_ = now;
      }
    break;
    } 
  }
  this->last_recompute_time_ = now;

}

} // namespace elero_wired
} // namespace esphome
