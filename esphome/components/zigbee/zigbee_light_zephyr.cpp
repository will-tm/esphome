#include "zigbee_light_zephyr.h"
#if defined(USE_ZIGBEE) && defined(USE_NRF52) && defined(USE_LIGHT)
#include "esphome/core/log.h"

extern "C" {
#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zb_nrf_platform.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_error_to_string.h>
}

namespace esphome::zigbee {

static const char *const TAG = "zigbee.light";

void ZigbeeLight::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Zigbee Light\n"
                "  Endpoint: %d, level %u, on_off %u",
                this->endpoint_, this->cluster_attributes_->current_level, this->cluster_attributes_->on_off);
}

void ZigbeeLight::setup() {
  this->parent_->add_callback(this->endpoint_, [this](zb_bufid_t bufid) { this->zcl_device_cb_(bufid); });

  // Register as listener for light state changes
  this->light_->add_remote_values_listener(this);
}

void ZigbeeLight::on_light_remote_values_update() {
  float brightness;
  this->light_->current_values_as_brightness(&brightness);
  bool is_on = this->light_->current_values.is_on();

  // Convert ESPHome brightness (0.0-1.0) to ZCL level (1-254, 0=off)
  zb_uint8_t level = is_on ? static_cast<zb_uint8_t>(brightness * 253.0f + 1.0f) : 0;

  this->cluster_attributes_->current_level = level;
  this->cluster_attributes_->on_off = is_on ? ZB_TRUE : ZB_FALSE;

  ESP_LOGD(TAG, "Light state changed: endpoint=%d, level=%u, on_off=%u", this->endpoint_, level,
           this->cluster_attributes_->on_off);

  // Update Level Control cluster
  ZB_ZCL_SET_ATTRIBUTE(this->endpoint_, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_CLUSTER_SERVER_ROLE,
                       ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID, &this->cluster_attributes_->current_level,
                       ZB_FALSE);

  // Update On/Off cluster
  ZB_ZCL_SET_ATTRIBUTE(this->endpoint_, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE,
                       ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &this->cluster_attributes_->on_off, ZB_FALSE);

  this->parent_->flush();
}

void ZigbeeLight::zcl_device_cb_(zb_bufid_t bufid) {
  zb_zcl_device_callback_param_t *p_device_cb_param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);
  zb_zcl_device_callback_id_t device_cb_id = p_device_cb_param->device_cb_id;

  ESP_LOGI(TAG, "Device callback received: cb_id=%d, endpoint=%d", device_cb_id, this->endpoint_);

  p_device_cb_param->status = RET_OK;

  switch (device_cb_id) {
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID: {
      zb_uint16_t cluster_id = p_device_cb_param->cb_param.set_attr_value_param.cluster_id;
      zb_uint16_t attr_id = p_device_cb_param->cb_param.set_attr_value_param.attr_id;
      ESP_LOGI(TAG, "SET_ATTR_VALUE: cluster=0x%04x, attr=0x%04x", cluster_id, attr_id);

      if (cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL) {
        if (attr_id == ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID) {
          uint8_t level = p_device_cb_param->cb_param.set_attr_value_param.values.data8;
          ESP_LOGI(TAG, "Level Control attribute setting to %u", level);

          this->defer([this, level]() {
            // Convert ZCL level (0-254) to ESPHome brightness (0.0-1.0)
            float brightness = (level > 0) ? (static_cast<float>(level - 1) / 253.0f) : 0.0f;
            bool is_on = level > 0;

            auto call = this->light_->make_call();
            if (is_on) {
              call.set_state(true);
              call.set_brightness(brightness);
            } else {
              call.set_state(false);
            }
            call.perform();
          });
        }
      } else if (cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF) {
        if (attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) {
          uint8_t on_off = p_device_cb_param->cb_param.set_attr_value_param.values.data8;
          ESP_LOGI(TAG, "On/Off attribute setting to %u", on_off);

          this->defer([this, on_off]() {
            auto call = this->light_->make_call();
            call.set_state(on_off != 0);
            call.perform();
          });
        }
      } else {
        ESP_LOGI(TAG, "Unhandled cluster: 0x%04x", cluster_id);
      }
      break;
    }
    case ZB_ZCL_LEVEL_CONTROL_SET_VALUE_CB_ID: {
      // ZBOSS uses this callback for Level Control commands
      zb_uint8_t level = p_device_cb_param->cb_param.level_control_set_value_param.new_value;
      ESP_LOGI(TAG, "LEVEL_CONTROL_SET_VALUE: level=%u", level);

      this->defer([this, level]() {
        float brightness = (level > 0) ? (static_cast<float>(level - 1) / 253.0f) : 0.0f;
        bool is_on = level > 0;

        auto call = this->light_->make_call();
        if (is_on) {
          call.set_state(true);
          call.set_brightness(brightness);
        } else {
          call.set_state(false);
        }
        call.perform();
      });
      break;
    }
    case ZB_ZCL_ON_OFF_WITH_EFFECT_VALUE_CB_ID: {
      // On/Off with effect command
      zb_uint8_t on_off = p_device_cb_param->cb_param.on_off_set_effect_value_param.on_off;
      ESP_LOGI(TAG, "ON_OFF_WITH_EFFECT: on_off=%u", on_off);

      this->defer([this, on_off]() {
        auto call = this->light_->make_call();
        call.set_state(on_off != 0);
        call.perform();
      });
      break;
    }
    default:
      ESP_LOGI(TAG, "Unhandled callback id: %d", device_cb_id);
      p_device_cb_param->status = RET_ERROR;
      break;
  }

  ESP_LOGD(TAG, "%s status: %hd", __func__, p_device_cb_param->status);
}

}  // namespace esphome::zigbee

#endif
