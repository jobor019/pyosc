import logging
from typing import List, Dict, Any

import zmq
import zmq.asyncio
from maxosc.caller import Caller
from maxosc.zmq_formatter import ZmqFormatter


class MaxZmq(Caller):
    def __init__(self, port: int, discard_duplicate_args: bool = True, flatten: bool = False,
                 str_encode_all_objects: bool = False):
        """ raises: IOError if fails to bind to socket """
        super().__init__(parse_parenthesis_as_list=False, discard_duplicate_args=discard_duplicate_args)
        self.logger = logging.getLogger(__name__)
        self.context: zmq.Context = zmq.Context()
        self.socket: zmq.Socket = self.context.socket(zmq.ROUTER)

        self.formatter = ZmqFormatter(flatten=flatten, str_encode_all_objects=str_encode_all_objects)

        self.running: bool = False

        address: str = "tcp://*:" + str(port)
        try:
            self.socket.bind(address)
        except zmq.error.ZMQError as e:
            raise IOError(e) from e

        self.logger.info(f"MaxZmq initialized on address {address}.")

    def run(self):
        poller: zmq.Poller = zmq.Poller()
        poller.register(self.socket, zmq.POLLIN)

        self.running = True
        while self.running:
            socks: Dict[zmq.Socket, int] = dict(poller.poll())

            if socks.get(self.socket) == zmq.POLLIN:
                message: List[bytes] = self.socket.recv_multipart()

                if len(message) != 2:
                    # TODO: Shouldn't raise this here - will crash entire loop
                    raise IOError("Invalid message format")

                sender: bytes = message[0]
                # TODO: Handle exceptions
                content: str = message[1].decode('utf-8')
                return_data: Any = self.call(content)
                return_message: List[bytes] = self.formatter.to_maxzmq_message(return_data, sender)
                self.socket.send_multipart(return_message)

    async def run_async(self):
        # TODO: Need to use zmq.asyncio.Context, zmq.asyncio.Poller and maybe even ZMQEventLoop.
        #  Look at zguide/../asyncio/rrbroker.py. Probably need its own class rather than its own function.
        raise NotImplementedError("Not implemented")
