from typing import Optional
import logging

from expansion.interfaces.public.ship import Ship
from expansion.interfaces.public.commutator import Commutator, ModuleInfo
from expansion.interfaces.public.engine import Engine


async def find_module(type: str, name: str, commutator: Commutator) -> Optional[ModuleInfo]:
    all_modules = await commutator.get_all_modules()
    if not all_modules:
        return None
    # Linear search is ok, because a number of modules is usually limited
    for desired_module_info in all_modules:
        if desired_module_info.type == type and desired_module_info.name == name:
            return desired_module_info
    return None


async def connect_to_engine(name: str, ship: Ship,
                            owner_name: Optional[str] = None) -> Optional[Engine]:
    """Find engine with the specified 'name' on the specified 'ship' and return
    engine instance. The optionally specified 'owner_name' will be used, to
    assign name for the module, otherwise the ship'd name will be used instead.
    """
    candidate = await find_module(type="Engine", name=name, commutator=ship.get_commutator())
    if not candidate:
        return None

    if not owner_name:
        owner_name = ship.get_name()
    engine: Engine = Engine(name=f"{owner_name}::engine_{name}")

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, engine)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the engine '{name}' on the ship '{ship.get_name()}': "
            f"{error}")
    return engine
