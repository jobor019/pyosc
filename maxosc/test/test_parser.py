from timeit import default_timer as timer

import numpy as np
import pytest
from maxosc.caller import Caller
from maxosc.exceptions import InvalidInputError, MaxOscError
from maxosc.parser import Parser, FunctionParam


def test_parseTree_valid_input():
    p = Parser()

    assert (p.process("hello") == ["hello"])
    assert (p.process("two args") == ["two", "args"])
    assert (p.process("'one string'") == ["one string"])
    assert (p.process("'  maintains       extra       spaces  '") == ["  maintains       extra       spaces  "])

    assert (p.process("1") == [1])
    assert (p.process("123") == [123])
    assert (p.process("-123") == [-123])

    assert (p.process("1.0") == [1.0])
    assert (p.process("123.") == [123.0])
    assert (p.process("-.123") == [-0.123])

    assert (p.process("[]") == [[]])
    assert (p.process("[1]") == [[1]])
    assert (p.process("[1 2 3]") == [[1, 2, 3]])
    assert (p.process("[[1 2 3] [4 5 6] [7 8 9]]") == [[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])
    assert (p.process("[   [1  2 3][4 5 6] [7 8 9]   ]  ") == [[[1, 2, 3], [4, 5, 6], [7, 8, 9]]])

    assert (p.process("()") == [()])
    assert (p.process("(1)") == [(1,)])
    assert (p.process("(1 2 3)") == [(1, 2, 3)])
    assert (p.process("(())") == [((),)])
    assert (p.process("()()") == [(), ()])

    assert (p.process("my_param=1234") == [FunctionParam("my_param", 1234)])
    assert (p.process("my_param=[complex (array of) informat 1 0 n.]")
            == [FunctionParam("my_param", ["complex", ("array", "of"), "informat", 1, 0, "n."])])

    assert (p.process("'my_param=1234'") == ["my_param=1234"])
    assert (p.process("'my_param= 1234'") == ["my_param= 1234"])
    assert (p.process("'my_param =1234'") == ["my_param =1234"])
    assert (p.process("'my_param = 1234'") == ["my_param = 1234"])
    assert (p.process("'{}= 1234'") == ["{}= 1234"])
    assert (p.process("ÅåÄäÆæÖöØøçÇéêèμィァアィイゥウェエォオカガキギク") == ["ÅåÄäÆæÖöØøçÇéêèμィァアィイゥウェエォオカガキギク"])

    # Max dicts
    # assert (json.dumps(p.process('{\"k\":\"v\"}')) == {"k":"v"})


def test_parseTree_invalid_input():
    p = Parser()

    with pytest.raises(InvalidInputError): p.process("myfunc = 123")

    # Unterminated content:
    with pytest.raises(InvalidInputError): p.process("[")
    with pytest.raises(InvalidInputError): p.process("[[]")
    with pytest.raises(InvalidInputError): p.process("[my content")
    with pytest.raises(InvalidInputError): p.process("(")
    with pytest.raises(InvalidInputError): p.process("'")
    with pytest.raises(InvalidInputError): p.process("'some text")