import logging
import re
from collections import deque
from enum import IntEnum
from typing import Any, Dict

from maxosc.exceptions import InvalidInputError


class TokenType(IntEnum):
    INT, FLOAT, BOOL, NONE, WS, LBRACK, RBRACK, LPAR, RPAR, \
    STR1, STR2, PARAM_NAME, RAW_STR, ANYTHING = range(14)


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
    FLOAT = r"-?(\d+\.[\d]*|\.[\d]+)(e-?\d+)?"
    BOOL = r'(true|false)'
    NONE = r'none'
    WS = r'\s+|^$'
    LPAR = r'\('
    RPAR = r'\)'
    LBRACK = r'\['
    RBRACK = r'\]'
    STR1 = r'"'
    STR2 = r'\''
    PARAM_NAME = r'[A-Za-z_][A-Za-z0-9_]*='
    RAW_STR = r'^[^=]+$'
    ANYTHING = r'.*'

    SEPARATORS = r'([A-Za-z0-9_]+=|["\'()[\]]|\s+)'

    def __init__(self, parse_parenthesis_as_list: bool = False):
        self.logger = logging.getLogger(__name__)
        self.tokens: [Token] = deque()
        self.results: [object] = []
        self.parse_parenthesis_as_list: bool = parse_parenthesis_as_list

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
            elif re.fullmatch(Parser.STR1, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.STR1, atom))
            elif re.fullmatch(Parser.STR2, atom, flags=re.IGNORECASE):
                self.tokens.append(Token(TokenType.STR2, atom))
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
            if self.parse_parenthesis_as_list:
                return tup_as_list
            else:
                return tuple(tup_as_list)
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
        except IndexError:
            raise InvalidInputError("incorrectly formatted input. Reached end of content before string was terminated.")

    def _peek_nws(self):
        while self.tokens:
            t: Token = self.tokens[0]
            if t.token_type != TokenType.WS:
                return t
            else:
                self.tokens.popleft()
        # The function should never reach this point assuming input is correctly formatted
        raise InvalidInputError("incorrectly formatted input. Reached end before list/tuple was terminated.")

    def _is_empty(self):
        return not len(self.tokens) or all([t.token_type == TokenType.WS for t in self.tokens])
