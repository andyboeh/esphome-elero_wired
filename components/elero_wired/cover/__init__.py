import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover

from esphome.const import (
    CONF_ID, 
    CONF_NAME, 
    CONF_OPEN_DURATION, 
    CONF_CLOSE_DURATION,
)

from .. import elero_wired_ns, EleroWired, CONF_ELERO_WIRED_ID

CODEOWNERS = ["@andyboeh"]
DEPENDENCIES = ["elero_wired"]

EleroWiredCover = elero_wired_ns.class_("EleroWiredCover", cover.Cover, cg.Component)

CONF_TILT_OPEN_DURATION = "tilt_open_duration"
CONF_TILT_CLOSE_DURATION = "tilt_close_duration"
CONF_EXTRA_WAIT_TIME = "extra_wait_time"
CONF_ENDSTOP_WAIT_TIME = "endstop_wait_time"

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(EleroWiredCover),
        cv.GenerateID(CONF_ELERO_WIRED_ID): cv.use_id(EleroWired),
        cv.Optional(CONF_OPEN_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_CLOSE_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TILT_OPEN_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TILT_CLOSE_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_EXTRA_WAIT_TIME, default="2s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_ENDSTOP_WAIT_TIME, default="120s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    parent = await cg.get_variable(config[CONF_ELERO_WIRED_ID])
    cg.add(var.set_elero_wired_parent(parent))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_tilt_open_duration(config[CONF_TILT_OPEN_DURATION]))
    cg.add(var.set_tilt_close_duration(config[CONF_TILT_CLOSE_DURATION]))
    cg.add(var.set_extra_wait_time(config[CONF_EXTRA_WAIT_TIME]))
    cg.add(var.set_endstop_wait_time(config[CONF_ENDSTOP_WAIT_TIME]))
