import json
import logging

from maxosc.Caller import Caller
from maxosc.Exceptions import InvalidInputError
from maxosc.MaxFormatter import MaxFormatter
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer
from pythonosc.udp_client import SimpleUDPClient


class MaxOsc(Caller):
    """Template class to extend when implementing new Max-OSC projects.

       This class enables the user to call any function in a child class and send any number of arguments using the
       syntax described in Parser.Caller.call to parse the content as native python objects
       (int, float, bool, str, None, list, tuple, dict).

       The class contains a method for formatting any native python content (apart from dicts) as a bach.llll and send
       back over OSC. It also forwards any warning (manually) caught in python to the address "/warning".

       All calls from max should be sent to the address `/call` and typically contain the return address as an argument.
    """

    def __init__(self, ip="127.0.0.1", port_in=8081, port_out=8080, parse_parenthesis_as_list=True):
        self.logger = logging.getLogger(__name__)
        super(MaxOsc, self).__init__(parse_parenthesis_as_list)
        self._client = SimpleUDPClient(ip, port_out)
        self._max_formatter: MaxFormatter = MaxFormatter()
        dispatcher = Dispatcher()
        dispatcher.map("/call", self._handle_msg)
        dispatcher.set_default_handler(self._default_handler)
        self._server = BlockingOSCUDPServer((ip, port_in), dispatcher)

        self.logger.info(f"MaxOsc initialized on ip {ip} with incoming port {port_in} and outgoing port {port_out}.")

    def _handle_msg(self, _address, *args):
        args_formatted: [str] = []
        for arg in args:
            if isinstance(arg, str) and " " in arg:
                args_formatted.append("'" + arg + "'")
            else:
                args_formatted.append(str(arg))
        args_str: str = " ".join([str(arg) for arg in args_formatted])
        try:
            self.call(args_str)
        except InvalidInputError:
            pass    # Warnings are sent through send_warning


    def _default_handler(self):
        warning = "OSCWarning: message received on unregistered address. No action performed."
        self.logger.warning(warning)
        self.send_warning(warning)

    def run(self) -> None:
        self._server.serve_forever()

    def send_raw(self, address: str, *args):
        self._client.send_message(address, args)

    def send_llll(self, address: str, *args):
        try:
            llll_string: str = self._max_formatter.format_llll(*args)
            self._client.send_message(address, llll_string)
        except InvalidInputError as e:
            self.send_warning(str(e))

    def parse_maxdict(self, dd: str) -> dict:
        return json.loads(dd)

    def send_maxdict(self, address: str, dd: dict):
        self._client.send_message(address, self._max_formatter.format_maxdict(dd))

    def send_warning(self, warning: str, *args, **kwargs):
        self.logger.warning(warning)
        self._client.send_message("/warning", warning)
