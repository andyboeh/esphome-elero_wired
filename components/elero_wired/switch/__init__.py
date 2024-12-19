import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from esphome.const import CONF_ID
from .. import elero_wired_ns, EleroWired, CONF_ELERO_WIRED_ID

DEPENDENCIES = ["elero_wired"]
CODEOWNERS = ["@andyboeh"]

EleroWiredSwitch = elero_wired_ns.class_("EleroWiredSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(EleroWiredSwitch),
        cv.GenerateID(CONF_ELERO_WIRED_ID): cv.use_id(EleroWired),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_ELERO_WIRED_ID])
    cg.add(var.set_elero_wired_parent(parent))
