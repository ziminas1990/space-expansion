from typing import Optional


class BaseModule:
    def __init__(self):
        pass

    def verify(self):
        pass

    def to_pod(self):
        self.verify()
        return {}
