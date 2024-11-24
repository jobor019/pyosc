import logging
from enum import IntEnum
from typing import Union

from deprecated.classic import deprecated
from maxosc.sender import Sender, OscSender


class MaxLogLevel(IntEnum):
    """ Enum to map Max `[print]` levels to python logging levels """

    INFO = 0
    WARNING = 1
    ERROR = 2
    DEBUG = 3

    def to_logging_level(self) -> int:
        if self == MaxLogLevel.DEBUG:
            return logging.DEBUG
        elif self == MaxLogLevel.INFO:
            return logging.INFO
        elif self == MaxLogLevel.WARNING:
            return logging.WARNING
        elif self == MaxLogLevel.ERROR:
            return logging.ERROR
        else:
            raise ValueError("Invalid log level. Must be 0 (print), 1 (warn), 2 (error), or 3 (debug).")


class OscLogForwarder(logging.Handler):
    def __init__(self, sender: OscSender, osc_log_address: str):
        super().__init__()
        self.sender: OscSender = sender
        self.osc_log_address: str = osc_log_address

    def emit(self, record):
        self.sender.send(self.osc_log_address, record.levelname.lower(), self.format(record))

    def set_logging_level(self, logging_level: int) -> None:
        self.setLevel(logging_level)

    def set_max_level(self, max_level: Union[int, MaxLogLevel]) -> None:
        self.setLevel(max_level.to_logging_level())


@deprecated(version='0.0.7', reason="Use the OscLogForwarder class instead.")
class OscLogHandler(logging.Handler):
    def __init__(self, sender: Sender, log_level: int = logging.INFO, log_format: str = '%(levelname)s %(message)s'):
        super().__init__()
        self.sender: Sender = sender
        self.setLevel(log_level)
        self.setFormatter(logging.Formatter(log_format))

    def emit(self, record: logging.LogRecord):
        self.sender.send_warning(self.format(record))
