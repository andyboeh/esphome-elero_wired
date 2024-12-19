import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_NAME,
)

CODEOWNERS = ["@andyboeh"]
MULTI_CONF = True

elero_wired_ns = cg.esphome_ns.namespace("elero_wired")
EleroWired = elero_wired_ns.class_("EleroWired", cg.Component)

CONF_PIN_OPEN = "open_pin"
CONF_PIN_CLOSE = "close_pin"
CONF_ELERO_WIRED_ID = "elero_wired_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EleroWired),
            cv.Required(CONF_PIN_OPEN): pins.gpio_output_pin_schema,
            cv.Required(CONF_PIN_CLOSE): pins.gpio_output_pin_schema,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    open_pin = await cg.gpio_pin_expression(config[CONF_PIN_OPEN])
    close_pin = await cg.gpio_pin_expression(config[CONF_PIN_CLOSE])
    cg.add(var.set_pin_open(open_pin))
    cg.add(var.set_pin_close(close_pin))

