import logging
from enum import IntEnum
from typing import Any

from maxosc.maxformatter import MaxFormatter
from pythonosc.udp_client import SimpleUDPClient


class SendFormat(IntEnum):
    RAW = 0
    FLATTEN = 1
    BACH_LLLL = 2


class Sender:
    def __init__(self, ip: str, port: int, send_format: SendFormat = SendFormat.FLATTEN,
                 warning_address: str = "/warning"):
        self.logger = logging.getLogger(__name__)
        self._send_format: SendFormat = send_format
        self._client: SimpleUDPClient = SimpleUDPClient(ip, port)
        self._warning_address: str = warning_address

    def send(self, address: str, *args):
        """ Raises: ValueError if invalid send format or trying to send custom class, etc.
                    InvalidInputError if unable to format as llll
                    TypeError if trying to send a dict"""
        if self._send_format == SendFormat.RAW:
            self._send_raw(address, *args)
        elif self._send_format == SendFormat.FLATTEN:
            self._send_flat(address, *args)
        elif self._send_format == SendFormat.BACH_LLLL:
            self._send_llll(address, *args)
        else:
            raise ValueError(f"Invalid send format '{self._send_format}'.")

    def _send_raw(self, address: str, *args):
        """ Raises: ValueError if invalid send format"""
        self._client.send_message(address, *args)

    def _send_flat(self, address: str, *args):
        """ Raises: ValueError if invalid send format or trying to send custom class, etc.
                    TypeError if trying to send a dict"""
        args_flattened: [Any] = MaxFormatter.flatten(*args)
        self._send_raw(address, args_flattened)

    def _send_llll(self, address: str, *args):
        """ Raises: InvalidInputError if unable to format as llll"""  # TODO: Catch at parent level
        llll_string: str = MaxFormatter.format_as_llll(*args)
        self._client.send_message(address, llll_string)

    def send_maxdict(self, address: str, dd: dict):
        # TODO: Implement full maxdict solution
        self._client.send_message(address, MaxFormatter.format_as_maxdict(dd))

    def send_warning(self, warning: str):
        self._client.send_message(self._warning_address, warning)
