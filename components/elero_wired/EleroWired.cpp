#include "EleroWired.h"
#include "esphome/components/elero_wired/cover/EleroWiredCover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace elero_wired {

static const char *const TAG = "elero_wired";

void EleroWired::loop() {
  if(this->pending_requests_ != PENDING_NONE) {
    uint32_t now = millis();
    
    if((now - ELERO_WIRED_RELAY_BLOCK_TIME) > this->last_relay_time_) {
      switch(this->pending_requests_) {
      case PENDING_SET_BOTH_RELAYS:
        this->pin_open_close_set();
        break;
      case PENDING_CLEAR_BOTH_RELAYS:
        this->pin_open_close_clear();
        break;
      default:
        break;
      }
    }
  }
}

void EleroWired::setup() {
  this->pin_open_->setup();
  this->pin_open_->digital_write(false);
  this->pin_close_->setup();
  this->pin_close_->digital_write(false);
  this->pin_open_state_ = false;
  this->pin_close_state_ = false;
}

void EleroWired::pin_open_set(void) {
  if(this->pin_close_state_) {
    ESP_LOGD(TAG, "Not setting OPEN relay because CLOSE relay is set!");
    return;
  }
  if(this->pin_open_ && !this->pin_open_state_) {
    ESP_LOGD(TAG, "Setting OPEN relay");
    this->pin_open_->digital_write(true);
    this->pin_open_state_ = true;
    this->last_relay_time_ = millis();
  }
}

void EleroWired::pin_close_set(void) {
  if(this->pin_open_state_) {
    ESP_LOGD(TAG, "Not setting CLOSE relay because OPEN relay is set!");
    return;
  }
  if(this->pin_close_ && !this->pin_close_state_) {
    ESP_LOGD(TAG, "Setting CLOSE relay");
    this->pin_close_->digital_write(true);
    this->pin_close_state_ = true;
    this->last_relay_time_ = millis();
  }
}

void EleroWired::pin_open_clear(void) {
  if(this->pin_open_ && this->pin_open_state_) {
    ESP_LOGD(TAG, "Clearing OPEN relay");
    this->pin_open_->digital_write(false);
    this->pin_open_state_ = false;
    this->last_relay_time_ = millis();
  }
}

void EleroWired::pin_close_clear(void) {
  if(this->pin_close_ && this->pin_close_state_) {
    ESP_LOGD(TAG, "Clearing CLOSE relay");
    this->pin_close_->digital_write(false);
    this->pin_close_state_ = false;
    this->last_relay_time_ = millis();
  }
}

void EleroWired::pin_open_close_set(void) {
  if(this->pin_close_ && this->pin_open_) {
    if(this->pin_open_state_ && this->pin_close_state_)
      return;

    if(this->cover_) {
      this->cover_->make_call().set_command_stop().perform();
    }

    if(this->pin_open_state_) {
      ESP_LOGD(TAG, "Open pin is set, deferring setting relays");
      this->pin_open_clear();
      this->pending_requests_ = PENDING_SET_BOTH_RELAYS;
      return;
    }
    if(this->pin_close_state_) {
      ESP_LOGD(TAG, "Close pin is set, deferring setting relays");
      this->pin_close_clear();
      this->pending_requests_ = PENDING_SET_BOTH_RELAYS;
      return;
    }

    if((millis() - ELERO_WIRED_RELAY_BLOCK_TIME) < this->last_relay_time_) {
      ESP_LOGD(TAG, "Minimum relay block time not reached, deferring setting relays");
      this->pending_requests_ = PENDING_SET_BOTH_RELAYS;
      return;
    }
    ESP_LOGD(TAG, "Setting BOTH relays");
    this->pin_close_->digital_write(true);
    this->pin_open_->digital_write(true);
    this->pin_open_state_ = true;
    this->pin_close_state_ = true;
    this->last_relay_time_ = millis();
    this->pending_requests_ = PENDING_NONE;
    if(this->cover_) {
      this->cover_->status_set_warning("Wireless mode activated, controls disabled.");
    }
  }
}

void EleroWired::pin_open_close_clear(void) {
  if(this->pin_close_ && this->pin_open_) {
    ESP_LOGD(TAG, "Clearing BOTH relays");
    this->pin_close_->digital_write(false);
    this->pin_open_->digital_write(false);
    this->pin_close_state_ = false;
    this->pin_open_state_ = false;
    this->last_relay_time_ = millis();
    this->pending_requests_ = PENDING_NONE;
    if(this->cover_) {
      this->cover_->status_clear_warning();
    }
  }
}

} // namespace elero_wired
} // namespace esphome
