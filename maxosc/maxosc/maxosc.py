import logging
from typing import Any, Optional

from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer

from maxosc.caller import Caller
from maxosc.exceptions import MaxOscError
from maxosc.maxformatter import MaxFormatter
from maxosc.oscloghandler import OscLogHandler
from maxosc.sender import Sender, SendFormat


class MaxOsc(Caller):
    # TODO: Update docstring
    """Template class to extend when implementing new Max-OSC projects.

       This class enables the user to call any function in a child class and send any number of arguments using the
       syntax described in Parser.Caller.call to parse the content as native python objects
       (int, float, bool, str, None, list, tuple, dict).

       The class contains a method for formatting any native python content (apart from dicts) as a bach.llll and send
       back over OSC. It lalso forwards any warning (manually) caught in python to the address "/warning".

       All calls from max should be sent to the address `/call` and typically contain the return address as an argument.
    """

    def __init__(self, ip: str = "127.0.0.1", port_in: int = 8081, port_out: int = 8080, recv_address: str = "/pyosc",
                 parse_parenthesis_as_list: bool = False, discard_duplicate_arguments: bool = True,
                 send_format: SendFormat = SendFormat.FLATTEN, osc_log_level: Optional[int] = logging.INFO):
        """

        :param ip:
        :param port_in:
        :param port_out:
        :param parse_parenthesis_as_list:
        :param send_format:
        :param osc_log_level: Forward log messages to Max. Will not forward if set to None. To forward only some messages, override _handle_max_osc_error, _handle_type_error or _handle_error
        """
        super(MaxOsc, self).__init__(parse_parenthesis_as_list, discard_duplicate_arguments)
        self.logger = logging.getLogger(__name__)
        self.sender: Sender = Sender(ip, port_out, send_format)
        if osc_log_level:
            handler: OscLogHandler = OscLogHandler(self.sender, osc_log_level)
            self.logger.addHandler(handler)

        dispatcher = Dispatcher()
        dispatcher.map(recv_address, self.main_callback)
        dispatcher.set_default_handler(self._default_handler)
        self._server = BlockingOSCUDPServer((ip, port_in), dispatcher)
        self.logger.info(f"MaxOsc initialized on ip {ip} with incoming port {port_in} and outgoing port {port_out}.")

    def main_callback(self, _recv_address: str, send_address: str, *args):
        args_formatted: str = MaxFormatter.format_as_string(*args)
        try:
            return_value: Any = self.call(args_formatted)
            self.sender.send(send_address, return_value)
        except MaxOscError as e:
            self._handle_max_osc_error(e)
        except TypeError as e:
            self._handle_type_error(e)
        except Exception as e:
            self._handle_error(e)

    def _default_error_handling(self, e: Exception):
        self.logger.error(e)
        self.logger.debug(repr(e))
        raise

    def _handle_max_osc_error(self, e: MaxOscError):
        self._default_error_handling(e)

    def _handle_type_error(self, e: TypeError):
        self._default_error_handling(e)

    def _handle_error(self, e: Exception):
        self._default_error_handling(e)

    def _default_handler(self):
        message = "[PyOsc Error]: Message received on unregistered address. No action was performed."
        self.logger.error(message)

    def run(self) -> None:
        self._server.serve_forever()
