import json
import re
from typing import Any, Dict

from maxosc.Exceptions import InvalidInputError


class MaxFormatter:

    def __init__(self):
        self.__string_list: [str] = []

    def format_llll(self, *args):
        """Formats any number of ints, floats, bools, None, lists and tuples into a llll-compatible string.

        Raises
        ------
        InvalidInputError
            If input contains any type not listed above.
        """
        self._parse_to_lll(*args)
        out: str = " ".join(self.__string_list)
        self.__string_list = []
        return out

    def _parse_to_lll(self, *args) -> [str]:
        for arg in args:
            if isinstance(arg, bool):
                self.__string_list.append(str(int(arg)))
            elif isinstance(arg, int) or isinstance(arg, float):
                self.__string_list.append(str(arg))
            elif isinstance(arg, str):
                self.__string_list.append("\"" + arg + "\"")
            elif isinstance(arg, list) or isinstance(arg, tuple):
                self.__string_list.append("(")
                self._parse_to_lll(*arg)
                self.__string_list.append(")")
            elif arg is None:
                self.__string_list.append("null")
            else:
                raise InvalidInputError(f"Type {type(arg)} is not supported by the llll syntax.")

    @staticmethod
    def format_maxdict(dd: Dict[Any, Any]) -> str:
        dd_str: str = json.dumps(dd, ensure_ascii=False, allow_nan=False)
        dd_escaped: str = re.sub(r'"', "\\\"", dd_str)
        return '"' + dd_escaped + '"'
