import json
from collections.abc import Iterable, Mapping
from typing import List, Any, Callable, Optional

import numpy as np


class ZmqFormatter:
    def __init__(self, flatten: bool = False, str_encode_all_objects: bool = False,
                 json_encode_func: Optional[Callable] = None):
        self.flatten = flatten
        self.str_encode_all_objects: bool = str_encode_all_objects
        self.json_encode_func: Optional[Callable] = json_encode_func

    def to_maxzmq_message(self, input_msg: Any, recipient: bytes) -> List[bytes]:
        data: List[str] = []
        types: List[str] = []
        self._parse(input_msg, data, types)  # recursively appending to `data` and `types`
        type_str: str = "".join(types)
        formatted_msg: List[str] = [type_str] + data
        return [recipient] + [bytes(s, 'utf-8') for s in formatted_msg]

    def _parse(self, elem: Any, parsed_data: List[str], types: List[str]):
        """ raises: TypeError for invalid data types, json encoding errors"""
        if isinstance(elem, int):
            parsed_data.append(str(elem))
            types.append('i')
        elif isinstance(elem, float):
            parsed_data.append(str(elem))
            types.append('f')
        elif isinstance(elem, str):
            parsed_data.append(elem)
            types.append('s')
        elif isinstance(elem, bool) or elem is None:
            parsed_data.append(str(elem))
            types.append('s')
        elif isinstance(elem, Mapping):
            parsed_data.append(self._parse_dict(elem))
            types.append('d')
        elif isinstance(elem, np.ndarray):
            parsed_data.append(self._parse_buffer(elem))
            types.append('b')
        elif isinstance(elem, Iterable):
            self._parse_list(elem, parsed_data, types)
        elif self.str_encode_all_objects:
            parsed_data.append(str(elem))
            types.append('s')
        else:
            raise TypeError(f"Invalid data type {type(elem)}")

    def _parse_list(self, iterable: Iterable[Any], parsed_data: List[str], types: List[str]):
        """ raises: TypeError for invalid data types, json encoding errors"""
        if not self.flatten:
            parsed_data.append("[")
            types.append('s')
        for elem in iterable:
            self._parse(elem, parsed_data, types)
        if not self.flatten:
            parsed_data.append("]")
            types.append('s')

    def _parse_dict(self, d: Mapping[Any, Any]) -> str:
        """ raises: TypeError for json encoding errors"""
        return json.dumps(d, default=self.json_encode_func)

    def _parse_buffer(self, b: np.ndarray) -> str:
        raise NotImplementedError("Not implemented yet")
