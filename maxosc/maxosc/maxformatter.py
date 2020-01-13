import collections.abc
import json
import re
from typing import Any, Dict, Iterable

from maxosc.exceptions import InvalidInputError


class MaxFormatter:
    DICT_DEPTH = float('inf')

    NONE_NAME = "null"


    @staticmethod
    def format_as_string(*args) -> str:
        args_as_strings: [str] = []
        for arg in args:
            if isinstance(arg, str) and " " in arg:
                args_as_strings.append("'" + arg + "'")
            else:
                args_as_strings.append(str(arg))
        args_string: str = " ".join([str(arg) for arg in args_as_strings])
        return args_string

    @staticmethod
    def format_as_llll(*args):
        """Formats any number of ints, floats, bools, None, lists and tuples into a bach.llll-compatible string.

        Raises
        ------
        InvalidInputError
            If input contains any type not listed above.
        """
        string_list: [str] = []
        MaxFormatter._parse_to_lll(string_list, *args)
        out: str = " ".join(string_list)
        return out

    @staticmethod
    def _parse_to_lll(string_list: [str], *args) -> [str]:
        for arg in args:
            if isinstance(arg, bool):
                string_list.append(str(int(arg)))
            elif isinstance(arg, int) or isinstance(arg, float):
                string_list.append(str(arg))
            elif isinstance(arg, str):
                string_list.append("\"" + arg + "\"")
            elif isinstance(arg, list) or isinstance(arg, tuple):
                string_list.append("(")
                MaxFormatter._parse_to_lll(string_list, *arg)
                string_list.append(")")
            elif arg is None:
                string_list.append(MaxFormatter.NONE_NAME)
            else:
                raise InvalidInputError(f"Type {type(arg)} is not supported by the llll syntax.")

    @staticmethod
    def format_as_maxdict(dd: Dict[Any, Any]) -> str:
        """ Parse a dict into a single string """
        dd_str: str = json.dumps(dd, ensure_ascii=False, allow_nan=False)
        dd_escaped: str = re.sub(r'"', "\\\"", dd_str)
        return dd_escaped

    @staticmethod
    def format_maxdict_large(dd: Dict[Any, Any]) -> [(str, str)]:
        """ Formats a large dict into several max-compatible key-value pairs.
         Will only handle dicts of dicts (of dicts of ...), not lists/tuples containing dicts."""
        parsed_dict: [(str, str)] = []
        MaxFormatter._format_maxdict_recursive(dd, parsed_dict, "somax")  # TODO: Not sure why this one says somax.
        return parsed_dict

    @staticmethod
    def _format_maxdict_recursive(dd: Dict[Any, Any], results: [(str, str)], parent_key: str):
        for k, v in dd.items():
            key: str = parent_key + "::" + k
            if isinstance(v, collections.abc.Mapping):
                MaxFormatter._format_maxdict_recursive(v, results, key)
            else:
                results.append((key, v))

    @staticmethod
    def parse_maxdict(dd: str) -> dict:
        return json.loads(dd)

    @staticmethod
    def depth(l: Any) -> int:
        return MaxFormatter._depth(l, 0)

    @staticmethod
    def _depth(l: Any, depth: int) -> int:
        if not isinstance(l, Iterable) or isinstance(l, (str, bytes)):
            return depth
        elif isinstance(l, collections.abc.Mapping):
            raise TypeError("Cannot parse dicts as return arguments")  # TODO: Handle when implem. dict strategy
        else:
            return max([MaxFormatter._depth(e, depth) for e in l])

    @staticmethod
    def flatten(l: Any) -> [Any]:
        """ Raises: TypeError if dict
            Note: Any nested empty lists etc will be removed. Nones will be replaced with the string 'null'"""
        return MaxFormatter._flatten([], l)

    @staticmethod
    def _flatten(flat_list: [Any], l: Any) -> [Any]:
        """ Raises: TypeError if dict"""
        if not isinstance(l, Iterable) or isinstance(l, (str, bytes)):
            if l is None:
                flat_list.append(MaxFormatter.NONE_NAME)
            else:
                flat_list.append(l)
        elif isinstance(l, collections.abc.Mapping):
            raise TypeError("Cannot parse dicts as return arguments")  # TODO: Handle when implem. dict strategy
        else:
            for e in l:
                MaxFormatter._flatten(flat_list, e)
        return flat_list
