#ifdef USE_ZEPHYR

#include "zephyr_pwm_output.h"
#include "esphome/core/log.h"

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

namespace esphome {
namespace zephyr_pwm {

static const char *const TAG = "zephyr_pwm";

void ZephyrPWM::setup() {
  // Get the PWM device from device tree
  this->pwm_dev_ = DEVICE_DT_GET(DT_NODELABEL(pwm0));

  if (!device_is_ready(static_cast<const device *>(this->pwm_dev_))) {
    ESP_LOGE(TAG, "PWM device is not ready");
    this->mark_failed();
    return;
  }

  // Calculate period in nanoseconds from frequency
  // period_ns = 1e9 / frequency
  this->period_ns_ = static_cast<uint32_t>(1000000000.0f / this->frequency_);

  ESP_LOGD(TAG, "PWM initialized: frequency=%.1f Hz, period=%u ns", this->frequency_, this->period_ns_);
}

void ZephyrPWM::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Zephyr PWM Output:\n"
                "  Pin: %u\n"
                "  Frequency: %.1f Hz\n"
                "  Inverted: %s",
                this->pin_number_, this->frequency_, YESNO(this->inverted_));
  LOG_FLOAT_OUTPUT(this);
}

void ZephyrPWM::write_state(float state) {
  if (this->pwm_dev_ == nullptr) {
    return;
  }

  // Apply inversion if needed
  if (this->inverted_) {
    state = 1.0f - state;
  }

  // Calculate pulse width in nanoseconds
  uint32_t pulse_ns = static_cast<uint32_t>(state * this->period_ns_);

  // Channel 0 is used (configured in device tree overlay)
  int ret = pwm_set(static_cast<const device *>(this->pwm_dev_), 0, this->period_ns_, pulse_ns, PWM_POLARITY_NORMAL);

  if (ret != 0) {
    ESP_LOGW(TAG, "Failed to set PWM: %d", ret);
  }
}

}  // namespace zephyr_pwm
}  // namespace esphome

#endif  // USE_ZEPHYR
