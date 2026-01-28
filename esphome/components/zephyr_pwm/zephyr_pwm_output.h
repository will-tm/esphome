#pragma once

#ifdef USE_ZEPHYR

#include "esphome/components/output/float_output.h"
#include "esphome/core/component.h"

namespace esphome {
namespace zephyr_pwm {

class ZephyrPWM : public output::FloatOutput, public Component {
 public:
  void set_pin_number(uint8_t pin_number) { this->pin_number_ = pin_number; }
  void set_frequency(float frequency) { this->frequency_ = frequency; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

 protected:
  void write_state(float state) override;

  uint8_t pin_number_{0};
  float frequency_{1000.0f};
  bool inverted_{false};
  uint32_t period_ns_{1000000};  // Period in nanoseconds (1MHz = 1000ns)
  const void *pwm_dev_{nullptr};
};

}  // namespace zephyr_pwm
}  // namespace esphome

#endif  // USE_ZEPHYR
