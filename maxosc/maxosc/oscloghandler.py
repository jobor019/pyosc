import logging

from maxosc.sender import Sender


class OscLogHandler(logging.Handler):
    def __init__(self, sender: Sender, log_level: int = logging.INFO, log_format: str = '%(levelname)s %(message)s'):
        super().__init__()
        self.sender: Sender = sender
        self.setLevel(log_level)
        self.setFormatter(logging.Formatter(log_format))

    def emit(self, record: logging.LogRecord):
        self.sender.send_warning(self.format(record))
