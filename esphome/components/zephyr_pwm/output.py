from esphome import pins
import esphome.codegen as cg
from esphome.components import output
from esphome.components.zephyr import zephyr_add_overlay, zephyr_add_prj_conf
import esphome.config_validation as cv
from esphome.const import CONF_FREQUENCY, CONF_ID, CONF_NUMBER, CONF_PIN

CODEOWNERS = ["@tomaszduda23"]
DEPENDENCIES = ["zephyr"]

zephyr_pwm_ns = cg.esphome_ns.namespace("zephyr_pwm")
ZephyrPWM = zephyr_pwm_ns.class_("ZephyrPWM", output.FloatOutput, cg.Component)

validate_frequency = cv.All(cv.frequency, cv.float_range(min=1.0, max=16000000.0))

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(ZephyrPWM),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_FREQUENCY, default="1kHz"): validate_frequency,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)

    pin_config = config[CONF_PIN]
    pin_number = pin_config[CONF_NUMBER]

    # nRF52 pin format: port * 32 + pin
    port = pin_number // 32
    pin_idx = pin_number % 32

    cg.add(var.set_pin_number(pin_number))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))

    # Check if pin is inverted
    if pin_config.get("inverted", False):
        cg.add(var.set_inverted(True))

    # Enable PWM in Kconfig
    zephyr_add_prj_conf("PWM", True)

    # Add device tree overlay for PWM
    # Using pwm0 with channel 0 for this pin
    # Delete existing pinctrl-1 (sleep state) if present and reset pinctrl-names
    zephyr_add_overlay(
        f"""
        &pinctrl {{
            pwm0_esphome: pwm0_esphome {{
                group1 {{
                    psels = <NRF_PSEL(PWM_OUT0, {port}, {pin_idx})>;
                }};
            }};
        }};

        &pwm0 {{
            status = "okay";
            pinctrl-0 = <&pwm0_esphome>;
            /delete-property/ pinctrl-1;
            pinctrl-names = "default";
        }};
        """
    )
