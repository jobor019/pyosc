import argparse
import asyncio
import ipaddress
import logging
import sys
import typing
from asyncio import BaseEventLoop
from typing import Optional, Callable, Awaitable, Union

from maxosc.caller import Caller
from maxosc.exceptions import MaxOscError
from maxosc.maxformatter import MaxFormatter
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import AsyncIOOSCUDPServer

from oscloghandler import MaxLogLevel
from oscloghandler import OscLogForwarder
from sender import OscSender


class Agent(Caller):
    """ High level interface class to use in combination with the Max object `[pyosc.agent]` """

    DEFAULT_IP = "127.0.0.1"
    DEFAULT_RECV_PORT = 8081
    DEFAULT_SEND_PORT = 8080

    DEFAULT_INPUT_ADDRESS = "/input"
    DEFAULT_OUTPUT_ADDRESS = "/output"
    DEFAULT_INTERNAL_ADDRESS = "/internal"

    class SendProtocol:
        STATUS = "status"
        INITIALIZED = "initialized"
        TERMINATED = "terminated"

    def __init__(self,
                 recv_port: int = DEFAULT_RECV_PORT,
                 send_port: int = DEFAULT_SEND_PORT,
                 ip: str = DEFAULT_IP,
                 log_level: Union[int, MaxLogLevel] = MaxLogLevel.INFO,
                 input_address: str = DEFAULT_INPUT_ADDRESS,
                 output_address: str = DEFAULT_OUTPUT_ADDRESS,
                 max_object_internal_address: str = DEFAULT_INTERNAL_ADDRESS,
                 log_to_osc: bool = False,
                 raise_exceptions: bool = True,
                 capture_termination_exceptions: bool = True,
                 discard_duplicate_args: bool = False,
                 status_callback_interval: float = 0.5):
        super().__init__(parse_parenthesis_as_list=False,
                         discard_duplicate_args=discard_duplicate_args)

        self._logger = logging.getLogger(__name__)
        self._log_config()
        self.loglevel(max_level=log_level.value)

        self.__running: bool = False

        self._recv_port: int = recv_port
        self._send_port: int = send_port
        self._ip: str = ip
        self._input_address: str = input_address
        self._output_address: str = output_address
        self._internal_address: str = max_object_internal_address
        self._raise_exceptions: bool = raise_exceptions
        self._capture_termination_exceptions: bool = capture_termination_exceptions
        self._status_callback_interval: float = status_callback_interval

        self._sender: OscSender = OscSender(ip, send_port)

        self._osc_log_handler: Optional[OscLogForwarder] = None
        if log_to_osc:
            self._osc_log_handler = OscLogForwarder(self._sender, self._internal_address)
            self._logger.addHandler(self._osc_log_handler)

        self._server: Optional[AsyncIOOSCUDPServer] = None

        self._async_targets: list[Callable[[], Awaitable[None]]] = []
        self.add_async_target(self._heartbeat_loop)

    ######################################################
    # OVERRIDABLE FUNCTIONS
    ######################################################

    @staticmethod
    def _log_config() -> None:
        logging.basicConfig(stream=sys.stdout, level=logging.INFO, format='[%(levelname)s]: %(message)s')

    def _on_initialize(self) -> None:
        pass

    def _on_terminate(self) -> None:
        pass

    ######################################################
    # PUBLIC (INTERNALLY ACCESSIBLE)
    ######################################################

    def start(self) -> None:
        if not self._capture_termination_exceptions:
            asyncio.run(self._run())

        else:
            try:
                asyncio.run(self._run())
            except OSError as e:
                self._logger.error(f"{str(e)}. Couldn't start '{self.__class__.__name__}'")
                self.terminate()
            except KeyboardInterrupt:
                self._logger.error(f"Terminating due to keyboard interrupt (SIGINT)")
                self.terminate()

    def add_async_target(self, func: Callable[[], Awaitable[None]]) -> None:
        if not self.__running:
            self._async_targets.append(func)
        else:
            raise RuntimeError("Cannot add async target while already running")

    def debug(self, msg: str) -> None:
        self._logger.debug(msg)

    def info(self, msg: str) -> None:
        self._logger.info(msg)

    def warning(self, msg: str) -> None:
        self._logger.warning(msg)

    def error(self, msg: str) -> None:
        self._logger.error(msg)

    def send(self, *args, osc_address: Optional[str] = None) -> None:
        osc_address = osc_address if osc_address is not None else self._output_address
        self._sender.send(osc_address, *args)

    ######################################################
    # PUBLIC (EXTERNALLY ACCESSIBLE)
    ######################################################

    def terminate(self) -> None:
        self._on_terminate()
        self.send(self.SendProtocol.TERMINATED, osc_address=self._internal_address)
        self._logger.info(f"Terminating '{self.__class__.__name__}'")
        self.__running = False

    def loglevel(self, max_level: Union[int, MaxLogLevel]) -> None:
        """ Note: levels passed as Max levels, i.e. 0 = print, 1 = warn, 2 = error, 3 = debug """
        try:
            self._logger.setLevel(MaxLogLevel(max_level).to_logging_level())
        except ValueError as e:
            self._logger.error(f"{e}. Log level was not changed")
            return

    ######################################################
    # PRIVATE
    ######################################################

    async def _run(self) -> None:
        self.__running = True

        self._logger.info(f"Starting '{self.__class__.__name__}' "
                          f"with recv_port={self._recv_port} and send_port={self._send_port}")

        # TODO: Does this work when it's single-threaded?
        # if self.osc_log_address:
        #     self.osc_log_handler = OscLogForwarder(self._sender, self.osc_log_address)
        #     self.logger.addHandler(self.osc_log_handler)

        osc_dispatcher: Dispatcher = Dispatcher()

        # python-osc will regexp-replace '*' with '[^/]*?/*', resulting in matches between /some_address and
        # /some_address2 even when the goal is to match only /some_address/some_child, hence the additional regex
        osc_dispatcher.map(f"{self._input_address}($|/*)", self._process_osc)
        osc_dispatcher.set_default_handler(self._unmatched_osc)

        self._server = AsyncIOOSCUDPServer((self._ip, self._recv_port),
                                           osc_dispatcher,
                                           typing.cast(BaseEventLoop, asyncio.get_event_loop()))
        transport, protocol = await self._server.create_serve_endpoint()
        self._on_initialize()
        self.send(self.SendProtocol.INITIALIZED, osc_address=self._internal_address)
        await asyncio.gather(*[f() for f in self._async_targets])
        transport.close()

    async def _heartbeat_loop(self) -> None:
        while self.__running:
            self.send(self.SendProtocol.STATUS, osc_address=self._internal_address)
            await asyncio.sleep(self._status_callback_interval)

    def _process_osc(self, _address: str, *args):
        try:
            self.call(MaxFormatter.format_as_string(*args))

        # Called with wrong number of arguments, with duplicate arguments or calling function that doesn't exist
        except MaxOscError as e:
            self._logger.error(e)
            self._logger.debug(repr(e))

        # Any other exception
        except Exception as e:
            self._logger.error(e)
            # self._logger.debug(repr(e))
            if self._raise_exceptions:
                raise

    def _unmatched_osc(self, address: str, *_args) -> None:
        self._logger.warning(f"The address '{address}' does not exist.")

    # ######################################################
    # ARGPARSE UTILITIES
    # ######################################################

    @staticmethod
    def parse_ip(ip: str) -> str:
        """ raises: ValuError if ip is invalid """
        ipaddress.ip_address(ip)
        return ip

    @staticmethod
    def default_argparse_arguments(parser: argparse.ArgumentParser,
                                   default_recv: int = DEFAULT_RECV_PORT,
                                   default_send: int = DEFAULT_SEND_PORT,
                                   default_ip: str = DEFAULT_IP,
                                   default_log_level: MaxLogLevel = MaxLogLevel.INFO,
                                   default_log_to_osc: bool = False) -> None:
        parser.add_argument('--recv_port', metavar='RECV_PORT', type=int,
                            help='input port used by the server',
                            default=default_recv)
        parser.add_argument('--send_port', metavar='SEND_PORT', type=int,
                            help='output port used by the server',
                            default=default_send)
        parser.add_argument('--ip', metavar='IP', type=Agent.parse_ip,
                            default=default_ip,
                            help='ip address of the max client')
        parser.add_argument('--log_level', metavar='LOG_LEVEL', type=lambda s: MaxLogLevel(int(s)),
                            help='log level (Max format): 0 (print), 1 (warn), 2 (error), or 3 (debug)',
                            default=default_log_level)
        parser.add_argument('--log_to_osc',
                            action='store_true',
                            default=default_log_to_osc,
                            help='Whether log entries should be output to OSC')
