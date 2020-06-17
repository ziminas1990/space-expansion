from typing import Dict, Type


def generate_name(cls: Type):
    """Generate a name for the instance of the specified 'cls'. The name will
    contain the class name and a unique identifier."""
    id: int = generate_name.next_ids[cls] if cls in generate_name.next_ids else 1
    generate_name.next_ids.update({cls: id + 1})
    return f"{cls.__name__}_{id}"


generate_name.next_ids = {}
