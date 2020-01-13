from abc import ABC
from typing import Any, Dict, Callable, List

from maxosc.exceptions import DuplicateKeyError, InvalidInputError, MaxOscError
from maxosc.parser import FunctionParam, Parser


class Caller(ABC):
    """Abstract class to make all functions of child class callable through a single function.

       By extending this class, any function of the child class can be called through the `call` function.
       An abstract method `send_warning` is also provided to let the user redirect the output of any warnings freely.
    """

    SELF_ARGUMENT_KEYWORD = "self"
    VARIABLE_ARGUMENTS_KEYWORD = "args"

    def __init__(self, parse_parenthesis_as_list: bool, discard_duplicate_args: bool):
        # TODO: Docstring
        """ discard_duplicate_args: if True: will discard any named argument (kwarg) that already exists,
        either as positional or optional argument. The leftmost value will be kept

        Example: when calling a function my_func(a,b,c):
            my_func a=1 a=2 a=3 b=4 c=5 will result in my_func(1,4,5)
            my_func 1 2 a=3 c=4         will result in my_func(1,2,4)
            my_func a=1 2 3 4           will still raise an error (2 will still be treated as first positional arg)

        If function accepts a variable number arguments, make sure that the last parameter is named `*args`.

        If false: will raise an error upon duplicate keys.
            """
        self.__parse_tree_parser: Parser = Parser(parse_parenthesis_as_list)
        self.discard_duplicate_args: bool = discard_duplicate_args

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
            func, args, kwargs = self._parse_args(parsed_args_list, self.discard_duplicate_args)
        except AttributeError as e:
            raise MaxOscError("[PyOsc Error]: A function with that name does not exist.") from e
        except DuplicateKeyError as e:
            raise MaxOscError("[PyOsc Error]: Multiple arguments with the same name/position were given.") from e
        except IndexError as e:
            raise MaxOscError("[PyOsc Error]: ") from e
        try:
            return func(*args, **kwargs)
        except TypeError as e:
            raise TypeError(f"[PyOsc Error]: {e}. The arguments for function {func.__name__} "
                            f"are: {func.__code__.co_varnames}") from e
        except Exception as e:
            raise type(e)(f"[PyOsc Error]: An exception occurred in function '{func.__name__}. {e}'") from e

    def _parse_args(self, parsed_args: [Any], discard_duplicate_args: bool) -> (Callable, List[Any], {str: Any}):
        # TODO: Docstring
        """ Raises: DuplicateKeyError if discard_duplicate_args"""
        func_name: str = parsed_args[0]
        func: Callable = getattr(self, func_name)
        args: [Any] = []
        kwargs: Dict[str: Any] = {}
        # Discards any duplicate arguments within kwargs, preferring the leftmost instance,
        # as well as any kwarg that already exists as a positional argument in args.
        if discard_duplicate_args:
            variable_names: [str] = list(func.__code__.co_varnames)
            if self.SELF_ARGUMENT_KEYWORD in variable_names:
                variable_names.remove(self.SELF_ARGUMENT_KEYWORD)
            for arg in parsed_args[1:]:
                if isinstance(arg, FunctionParam):
                    if arg.name not in kwargs and arg.name in variable_names:
                        kwargs[arg.name] = arg.value
                        variable_names.remove(arg.name)
                else:
                    if variable_names:
                        args.append(arg)
                        # If function accepts a variable number of arguments, don't discard the last name (*args)
                        if not variable_names[0] == self.VARIABLE_ARGUMENTS_KEYWORD:
                            variable_names.pop(0)
        else:
            for arg in parsed_args[1:]:
                if isinstance(arg, FunctionParam):
                    if arg.name not in kwargs:
                        kwargs[arg.name] = arg.value
                    else:
                        raise DuplicateKeyError(f"Got multiple values for argument '{arg.name}'.")
                else:
                    args.append(arg)
        return func, args, kwargs
