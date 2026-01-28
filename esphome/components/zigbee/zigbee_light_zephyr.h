#pragma once

#include "esphome/components/zigbee/zigbee_zephyr.h"
#if defined(USE_ZIGBEE) && defined(USE_NRF52) && defined(USE_LIGHT)
#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"
extern "C" {
#include <zboss_api.h>
#include <zboss_api_addons.h>
}

namespace esphome::zigbee {

// Level Control cluster attributes structure
struct LevelAttrs {
  zb_uint8_t current_level;       // 0-254 (0 = off, 254 = max)
  zb_uint16_t remaining_time;     // Time remaining for transition
  zb_uint8_t min_level;           // Minimum level (usually 1)
  zb_uint8_t max_level;           // Maximum level (usually 254)
  zb_bool_t on_off;               // On/Off cluster state
};

// Attribute descriptors for min/max level (not provided by ZBOSS)
#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_LEVEL_CONTROL_MIN_LEVEL_ID(data_ptr) \
  { \
    ZB_ZCL_ATTR_LEVEL_CONTROL_MIN_LEVEL_ID, ZB_ZCL_ATTR_TYPE_U8, ZB_ZCL_ATTR_ACCESS_READ_ONLY, \
        (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *) (data_ptr) \
  }

#define ZB_SET_ATTR_DESCR_WITH_ZB_ZCL_ATTR_LEVEL_CONTROL_MAX_LEVEL_ID(data_ptr) \
  { \
    ZB_ZCL_ATTR_LEVEL_CONTROL_MAX_LEVEL_ID, ZB_ZCL_ATTR_TYPE_U8, ZB_ZCL_ATTR_ACCESS_READ_ONLY, \
        (ZB_ZCL_NON_MANUFACTURER_SPECIFIC), (void *) (data_ptr) \
  }

// Macro to declare Level Control attribute list
#define ESPHOME_ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(attr_list, current_level, remaining_time, min_level, \
                                                          max_level) \
  ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_LEVEL_CONTROL) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID, (current_level)) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_LEVEL_CONTROL_REMAINING_TIME_ID, (remaining_time)) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_LEVEL_CONTROL_MIN_LEVEL_ID, (min_level)) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_LEVEL_CONTROL_MAX_LEVEL_ID, (max_level)) \
  ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST

// Macro to declare On/Off attribute list
#define ESPHOME_ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(attr_list, on_off) \
  ZB_ZCL_START_DECLARE_ATTRIB_LIST_CLUSTER_REVISION(attr_list, ZB_ZCL_ON_OFF) \
  ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, (on_off)) \
  ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST

class ZigbeeLight : public ZigbeeEntity, public Component, public light::LightRemoteValuesListener {
 public:
  explicit ZigbeeLight(light::LightState *light) : light_(light) {}
  void set_cluster_attributes(LevelAttrs &cluster_attributes) { this->cluster_attributes_ = &cluster_attributes; }

  void setup() override;

  void dump_config() override;

  // LightRemoteValuesListener interface - called when light state changes
  void on_light_remote_values_update() override;

 protected:
  void zcl_device_cb_(zb_bufid_t bufid);

  LevelAttrs *cluster_attributes_{nullptr};
  light::LightState *light_;
};

}  // namespace esphome::zigbee
#endif
