from abc import ABC
from typing import Any, Dict, Callable, List

from maxosc.Exceptions import DuplicateKeyError, InvalidInputError, MaxOscError
from maxosc.Parser import FunctionParam, Parser


class Caller(ABC):
    """Abstract class to make all functions of child class callable through a single function.

       By extending this class, any function of the child class can be called through the `call` function.
       An abstract method `send_warning` is also provided to let the user redirect the output of any warnings freely.
    """

    def __init__(self, parse_parenthesis_as_list: bool):
        self.__parse_tree_parser: Parser = Parser(parse_parenthesis_as_list)

    def call(self, string: str) -> Any:
        # TODO: Update docstring
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
        Given an instance `m` of a class `MyClass` with function my_func(a, b) (extending `Caller`):
        >>> m = MyClass(Caller)
        >>> m.call("my_func 1 2")
        >>> m.call("my_func 1 b=2")
        >>> m.call("my_func a=1 b=2")
        are all equivalent to the call `m.my_func(1, 2)`

        More complex types are supported:
        >>> m.call("my_func [1 2 3 4]")
        (equiv: m.my_func([1, 2, 3, 4]))
        >>> m.call("my_func (1 2 3 4)")
        (equiv: m.my_func((1, 2, 3, 4)))
        >>> m.call("my_func {1:2 3:4}")
        (equiv: m.my_func({1: 2, 3: 4}))
        >>> m.call("my_func 'long string of words'")
        (equiv: m.my_func('long string of words'))
        as well as any combination of them:
        >>> m.call("my_func [(1 2) {k:[3.4 5]} true false () none]")
        (equiv: m.my_func([(1, 2), {"k": [3.4, 5]}, True, False, (), None]))

        Parameters
        ----------
        string : str
            function call to parse, formatted as above.

        Returns
        -------
        None

        Raises
        ------
        MaxOscError: Raised if input in any way is incorrectly formatted, if function doesn't exist
                     or has invalid argument names/values.
        Exception: Any uncaught exception by the function called will be raised here.

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
            raise MaxOscError(f"[PyOsc Error]: {e}") from e

        try:
            func, args, kwargs = self._parse_args(parsed_args_list)
        except AttributeError as e:
            raise MaxOscError("[PyOsc Error]: A function with that name does not exist.") from e
        except DuplicateKeyError as e:
            raise MaxOscError("[PyOsc Error]: Multiple arguments with the same name/position were given.") from e
        try:
            return func(*args, **kwargs)
        except TypeError as e:
            raise TypeError(f"[PyOsc Error]: {e}. The arguments for function {func.__name__} "
                            f"are: {func.__code__.co_varnames[1:]}") from e
        except Exception as e:
            raise type(e)(f"[PyOsc Error]: An exception occurred in function '{func.__name__}. {e}'") from e

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
                    raise DuplicateKeyError(f"Got multiple values for argument '{arg.name}'.")
            else:
                args.append(arg)
        return func, args, kwargs
