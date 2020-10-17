from .queued_terminal import QueuedTerminal, Channel


class QueuedSocket(QueuedTerminal):
    """What the hell? Why QueuedSocket is a QueuedTerminal?
    Because their interfaces and behavior (and, subsequently, implementation)
    are the same. So, what is the different?
    Well, the 'QueuedTerminal' should better be used as a base class. But the
    'QueuedSocket' should be used as standalone object, when inheriting
    'QueuedTerminal' makes no sense. In other words, this class has been
    introduced to make client's code more understandable.
    """
    pass
