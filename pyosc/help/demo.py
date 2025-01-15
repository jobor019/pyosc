import argparse
import random
from typing import Optional

from maxosc import Agent, MaxLogLevel


class BasicAdder(Agent):
    def __init__(self, rhs: int, recv_port: int, send_port: int, ip: str, log_level: MaxLogLevel, log_to_osc: bool):
        super().__init__(recv_port=recv_port,
                         send_port=send_port,
                         ip=ip,
                         log_level=log_level,
                         log_to_osc=log_to_osc)
        self._rhs = rhs
        self._count = 0

    ######################################################
    # Basic Functions
    ######################################################

    def add(self, v: int) -> None:
        self.send("add", self._rhs + v)

    def rhs(self, v: Optional[int] = None) -> None:
        if v is not None:  # setter
            self._rhs = v

        self.send("rhs", self._rhs)

    def sumlist(self, *args) -> None:
        self.send("sumlist", sum(args))

    ######################################################
    # Logging
    ######################################################

    def throw(self) -> None:
        raise RuntimeError("This functions throws an uncaught exception")

    def throwsafe(self) -> None:
        try:
            a = 1 / 0
        except ZeroDivisionError:
            self.error("This is a caught exception")

    def warnme(self) -> None:
        self.warning("This is a warning")

    def informme(self) -> None:
        self.info("This is some info")

    def debugme(self) -> None:
        self.debug("This is a debug message")

    def normalprint(self) -> None:
        print("This is a normal print message")

    ######################################################
    # Maximum OSC Message Limit
    ######################################################

    def newlist(self, n: int) -> None:
        self.send("newlist", list(range(n)))

    def newstring(self, n: int) -> None:
        self.send("newstring", ''.join(chr(random.randint(33, 126)) for _ in range(n)))

    ######################################################
    # Maximum Number of Incoming / Outgoing OSC Messages
    ######################################################

    def floodmax(self, n: int) -> None:
        for _ in range(n):
            self.send("floodmax", "bang")

    def floodcount(self) -> None:
        self.send("floodcount", self._count)

    def floodreset(self) -> None:
        self._count = 0
        self.send("floodcount", self._count)

    def floodpython(self, *_args) -> None:
        self._count += 1


    ######################################################
    # Multi Communication (Targeted Messages)
    ######################################################

    def addwithid(self, id: str, v: int) -> None:
        self.send(id, "addwithid", self._rhs + v)

    ######################################################
    # Rawsend (Handling functions with reserved keywords)
    ######################################################

    def initialize(self) -> None:
        self.info("This function has the same name as a reserved keyword in bis.oscagent")



if __name__ == '__main__':
    parser: argparse.ArgumentParser = argparse.ArgumentParser()
    parser.add_argument('--rhs', metavar='RHS', type=int,
                        help='rhs value',
                        default=0)
    Agent.append_default_argparse_args(parser)

    args = parser.parse_args()
    BasicAdder(rhs=args.rhs,
               recv_port=args.recv_port,
               send_port=args.send_port,
               ip=args.ip,
               log_level=args.log_level,
               log_to_osc=args.log_to_osc).start()
