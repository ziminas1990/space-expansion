from typing import Dict

from .resource_container import ResourceContainerBlueprint


resource_containers_blueprints: Dict[str, ResourceContainerBlueprint] = {
    "tiny": ResourceContainerBlueprint(name="Tiny Resource Container",
                                       volume=20),
    "small": ResourceContainerBlueprint(name="Small Resource Container",
                                        volume=125),
    "medium": ResourceContainerBlueprint(name="Medium Resource Container",
                                         volume=500),
    "large": ResourceContainerBlueprint(name="Large Resource Container",
                                        volume=1600)
}
