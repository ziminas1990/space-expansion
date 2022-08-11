from typing import Any, List, Optional


def get_message_field(message: Any, path: List[str]) -> Optional[Any]:
    """Read a field with the specified 'path' in the specified 'message'

     login_response = extract_message(response, ["access", "login"])

     In this case function will check, that 'response' has 'access' field,
     and 'access' field has 'login' field. If all is Ture, it will return
     'login' field. Otherwise, 'None' will be returned.

     This function assumes, that message and all it's nested messages have
     'oneof choice' field in .proto file.
    """
    if not message:
        return None

    field = message
    for name in path:
        if field.WhichOneof('choice') != name:
            return None
        field = getattr(field, name)
    return field
