from expansion.transport.terminal import BufferedTerminal

from .commutator import Commutator


class Ship(BufferedTerminal):

    def __init__(self):
        super().__init__()
        self.commutator: Commutator = Commutator()
