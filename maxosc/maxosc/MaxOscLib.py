import json
import logging
import re
from abc import ABC, abstractmethod
from collections import deque
from enum import IntEnum
from typing import List, Any, Dict, Callable


class Caller(ABC):
    """Abstract class to make all functions of child class callable through a single function.

       By extending this class, any function of the child class can be called through the `call` function.
       An abstract method `send_warning` is also provided to let the user redirect the output of any warnings freely.
    """

    def __init__(self):
        self.__parse_tree_parser: Parser = Parser()

    def call(self, string: str):
        """Parses a string and if content of string matches a function, calls that function with provided arguments.

       The elements of the string should be space-separated, any other separator (comma, semicolon) will be
       treated as a string. The first element is the function to call and any following item will be treated as
       a parameter.

       The following types are supported:
          int, float, bool, str, list, dict, tuple, None.

       Parameters may be named or unnamed, e.g. `f 123` and `f x=123` are equally valid, assuming the function
       `f` has a single parameter named `x`. Lists, tuples and dicts will be treated as single parameters, e.g.
       and each element should be separated with a space. A space is not required before/after the leading/trailing
       bracket, parenthesis or brace. Similarly, strings containing multiple words can created using single quotes.

        Examples
        --------
        Given an instance `my_class` of a class `MyClass` with function my_func(a, b=None) (extending `Caller`):

        >>> my_class.call("my_func 1 2")
        >>> my_class.call("my_func 1 b=2")
        >>> my_class.call("my_func a=1 b=2")
        are all equivalent to the call `my_class.my_func(1, 2)`

        More complex types are supported:
        >>> my_class.call("my_func [1 2 3 4]")
        (equiv: my_class.my_func([1, 2, 3, 4]))
        >>> my_class.call("my_func (1 2 3 4)")
        (equiv: my_class.my_func((1, 2, 3, 4)))
        >>> my_class.call("my_func {1:2 3:4}")
        (equiv: my_class.my_func({1: 2, 3: 4}))
        >>> my_class.call("my_func 'long string of words'")
        (equiv: my_class.my_func('long string of words'))
        as well as any combination of them:
        >>> my_class.call("my_func [(1 2) {k:[3.4 5]} true false () none]")
        (equiv: my_class.my_func([(1, 2), {"k": [3.4, 5]}, True, False, (), None]))

        Parameters
        ----------
        string : str
            function call to parse, formatted as above.

        Returns
        -------
        None

        Raises
        ------
        Will not raise any exceptions, but any invalid input triggers a call with the exception content to
        the (user-defined) function `send_warning`.

        Notes
        -----
        Note that this function will never return any value and can not be modified to do so. A
        ny function call must be self contained.

        Also note that the symbols `=` (equal) `:` (colon) and `'` (single quote) are not allowed in strings,
        as they are use for named parameters, dictionaries and multiword strings respectively.

        The parser is not case sensitive for booleans/None and can handle any number of whitespaces as separators.
        """
        try:
            parsed_args_list = self.__parse_tree_parser.process(string)
        except InvalidInputError as e:
            self.send_warning(f"Error: {e}")
            return
        except TypeError as e:
            self.send_warning(f"(pyosc) Error: {e}. The key of a dict must be a primitive or a tuple.")
            return

        try:
            func, args, kwargs = self._parse_args(parsed_args_list)
        except AttributeError as e:
            self.send_warning(f"(pyosc) Error: {e}")
            return
        except DuplicateKeyError as e:
            self.send_warning(f"(pyosc) Error: {e}")
            return
        except TypeError as e:
            self.send_warning(f"(pyosc) Error: {e}. Likely, the first argument does match a function name.")
            return

        try:
            func(*args, **kwargs)
        except TypeError as e:
            self.send_warning(f"(pyosc) Error: {e}. The arguments for function {func.__name__} "
                              f"are: {func.__code__.co_varnames[1:]}")
            raise
        except Exception as e:
            self.send_warning(f"(pyosc) Error: {e}")
            raise

    @abstractmethod
    def send_warning(self, warning: str, *args, **kwargs):
        raise NotImplementedError("No send_warning function has been implemented")

    def _parse_args(self, parsed_args: [Any]) -> (Callable, List[Any], {str: Any}):
        func_name: str = parsed_args[0]
        func: Callable = getattr(self, func_name)
        args: [Any] = []
        kwargs: Dict[str: Any] = {}
        for arg in parsed_args[1:]:
            if isinstance(arg, FunctionParam):
                if arg.name not in kwargs:
                    kwargs[arg.name] = arg.value
                else:
                    raise DuplicateKeyError(f"got multiple values for argument '{arg.name}'.")
            else:
                args.append(arg)
        return func, args, kwargs


class InvalidInputError(Exception):
    def __init__(self, message):
        super().__init__(message)


class DuplicateKeyError(Exception):
    def __init__(self, message):
        super().__init__(message)


class TokenType(IntEnum):
    INT, FLOAT, BOOL, NONE, WS, LPAR, RPAR, LBRACK, RBRACK, LBRACE, RBRACE, \
    STR1, STR2, COLON, PARAM_NAME, RAW_STR, ANYTHING = range(17)


class Token:
    def __init__(self, token: TokenType, value: str):
        self.token_type: TokenType = token
        self.value = value

    def __repr__(self):
        return "(" + str(self.token_type) + ",'" + self.value + "')"


class FunctionParam:
    def __init__(self, name: str, value: Any):
        self.name: str = name
        self.value: Any = value

    def __eq__(self, other):
        return self.name == other.name and self.value == other.value


class Parser:
    INT = r"-?\d+"
    FLOAT = r"-?(\d+\.[\d]*|\.[\d]+)"
    BOOL = r'(true|false)'
    NONE = r'none'
    WS = r'\s+|^$'
    LPAR = r'\('
    RPAR = r'\)'
    LBRACK = r'\['
    RBRACK = r'\]'
    LBRACE = r'\{'
    RBRACE = r'\}'
    STR1 = r'"'
    STR2 = r'\''
    COLON = r':'
    PARAM_NAME = r'[A-Za-z_][A-Za-z0-9_]*='
    RAW_STR = r'^[^=:]+$'
    ANYTHING = r'.*'

    SEPARATORS = r'([A-Za-z0-9_]+=|[:"\'{}()[\]]|\s+)'

    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.tokens: [Token] = deque()
        self.results: [object] = []

    def process(self, text) -> [object]:
        self._reset()
        self._lex(text)
        self._parse()
        return self.results

    def _reset(self):
        self.tokens: [Token] = deque()
        self.results: [object] = []

    def _lex(self, string: str) -> [Token]:
        str_list: [str] = re.split(Parser.SEPARATORS, string)
        for atom in str_list:
            if re.fullmatch(Parser.INT, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.INT, atom))
            elif re.fullmatch(Parser.FLOAT, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.FLOAT, atom))
            elif re.fullmatch(Parser.BOOL, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.BOOL, atom))
            elif re.fullmatch(Parser.NONE, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.NONE, atom))
            elif re.fullmatch(Parser.WS, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.WS, atom))
            elif re.fullmatch(Parser.LPAR, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.LPAR, atom))
            elif re.fullmatch(Parser.RPAR, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.RPAR, atom))
            elif re.fullmatch(Parser.LBRACK, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.LBRACK, atom))
            elif re.fullmatch(Parser.RBRACK, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.RBRACK, atom))
            elif re.fullmatch(Parser.LBRACE, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.LBRACE, atom))
            elif re.fullmatch(Parser.RBRACE, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.RBRACE, atom))
            elif re.fullmatch(Parser.STR1, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.STR1, atom))
            elif re.fullmatch(Parser.STR2, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.STR2, atom))
            elif re.fullmatch(Parser.COLON, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.COLON, atom))
            elif re.fullmatch(Parser.PARAM_NAME, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.PARAM_NAME, atom))
            elif re.fullmatch(Parser.RAW_STR, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.RAW_STR, atom))
            elif re.fullmatch(Parser.ANYTHING, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.ANYTHING, atom))
            else:
                raise InvalidInputError(f"incorrectly formatted input. '{atom}' is not valid in this position.")
        return self.tokens

    def _parse(self):
        self._params()

    def _params(self):
        self._param()
        if not self._is_empty():
            self._params()

    def _param(self):
        # named_param
        if self._peek_nws().token_type == TokenType.PARAM_NAME:
            param_name_token = self._pop_nws()
            param_name = param_name_token.value.strip("=")
            param_value = self._obj()
            self.results.append(FunctionParam(param_name, param_value))
        else:
            self.results.append(self._obj())

    def _obj(self) -> object:
        t: Token = self._pop_nws()
        if t.token_type == TokenType.INT:
            return int(t.value)
        elif t.token_type == TokenType.FLOAT:
            return float(t.value)
        elif t.token_type == TokenType.RAW_STR:
            return str(t.value)
        elif t.token_type == TokenType.BOOL:
            return self._bool(t.value)
        elif t.token_type == TokenType.NONE:
            return None
        elif t.token_type == TokenType.LBRACK:
            l: [object] = []
            self._list(l)
            return l
        elif t.token_type == TokenType.LPAR:
            tup_as_list: [object] = []
            self._tuple(tup_as_list)
            return tuple(tup_as_list)
        elif t.token_type == TokenType.LBRACE:
            d: Dict = {}
            self._dict(d)
            return d
        elif t.token_type == TokenType.STR1:
            str_list: [str] = []
            next_token: Token = self._pop_ws()
            while next_token.token_type != TokenType.STR1:
                str_list.append(next_token.value)
                next_token = self._pop_ws()
            return "".join(str_list)
        elif t.token_type == TokenType.STR2:
            str_list: [str] = []
            next_token: Token = self._pop_ws()
            while next_token.token_type != TokenType.STR2:
                str_list.append(next_token.value)
                next_token = self._pop_ws()
            return "".join(str_list)
        else:
            raise InvalidInputError(f"incorrectly formatted input. '{t.value}' is not valid in this position.")

    def _bool(self, string: str):
        return string.lower() == 'true'

    def _list(self, l: [object]):
        t: Token = self._peek_nws()
        while t.token_type != TokenType.RBRACK:
            l.append(self._obj())
            t = self._peek_nws()
        self._pop_nws()

    def _tuple(self, l: [object]):
        t: Token = self._peek_nws()
        while t.token_type != TokenType.RPAR:
            l.append(self._obj())
            t = self._peek_nws()
        self._pop_nws()

    def _dict(self, d: Dict):
        t: Token = self._peek_nws()
        while t.token_type != TokenType.RBRACE:
            self._kvpair(d)
            t = self._peek_nws()
        self._pop_nws()

    def _kvpair(self, d: Dict):
        k = self._obj()
        if self._pop_nws().token_type != TokenType.COLON:
            raise InvalidInputError("incorrectly formatted input. Missing content in dictionary.")
        v = self._obj()
        if k not in d:
            d[k] = v
        else:
            raise InvalidInputError("incorrectly formatted input. Duplicate keys found in dict.")

    def _pop_nws(self):
        while self.tokens:
            t: Token = self.tokens.popleft()
            if t.token_type != TokenType.WS:
                return t
        raise InvalidInputError("incorrectly formatted input. Reached end of content prematurely.")

    def _pop_ws(self):
        try:
            return self.tokens.popleft()
        except IndexError as e:
            raise InvalidInputError("incorrectly formatted input. Reached end of content before string was terminated.")

    def _peek_nws(self):
        while self.tokens:
            t: Token = self.tokens[0]
            if t.token_type != TokenType.WS:
                return t
            else:
                self.tokens.popleft()
        # The function should never reach this point assuming input is correctly formatted
        raise InvalidInputError("incorrectly formatted input. Reached end before list/tuple/dict was completed.")

    def _is_empty(self):
        return not len(self.tokens) or all([t.token_type == TokenType.WS for t in self.tokens])


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
