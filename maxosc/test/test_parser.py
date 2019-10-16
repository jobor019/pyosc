from timeit import default_timer as timer

import numpy as np
import pytest
from maxosc.Caller import Caller
from maxosc.Exceptions import InvalidInputError
from maxosc.Parser import Parser, FunctionParam


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


def test_caller_valid_input():
    t = TestClass()

    t.call("func_no_args")
    assert (t.var == 0 and t.warning_count == 0)

    t.call("func_one_mand 1")
    assert (t.var == 1 and t.warning_count == 0)
    t.call("func_one_mand mand=True")
    assert (t.var == True and t.warning_count == 0)
    t.call("func_one_mand mand=     False")
    assert (t.var == False and t.warning_count == 0)
    t.call("func_one_mand []")
    assert (t.var == [] and t.warning_count == 0)
    t.call("func_one_mand [1 2 3]")

    t.call("func_one_opt")
    assert (t.var == None and t.warning_count == 0)
    t.call("func_one_opt 4")
    assert (t.var == 4 and t.warning_count == 0)
    t.call("func_one_opt opt=5")
    assert (t.var == 5 and t.warning_count == 0)

    t.call("func_one_each 6")
    assert (t.var == (6, None) and t.warning_count == 0)
    t.call("func_one_each 7 8")
    assert (t.var == (7, 8) and t.warning_count == 0)
    t.call("func_one_each 9 opt=10")
    assert (t.var == (9, 10) and t.warning_count == 0)
    t.call("func_one_each mand=11 opt=12")
    assert (t.var == (11, 12) and t.warning_count == 0)
    t.call("func_one_each opt=-1 mand=-2")
    assert (t.var == (-2, -1) and t.warning_count == 0)
    t.call("func_one_each opt=-2 -3")
    assert (t.var == (-3, -2) and t.warning_count == 0)

    t.call("func_two_each 13 14")
    assert (t.var == (13, 14, None, None) and t.warning_count == 0)
    t.call("func_two_each 15 16 17")
    assert (t.var == (15, 16, 17, None) and t.warning_count == 0)
    t.call("func_two_each 18 19 20 21")
    assert (t.var == (18, 19, 20, 21) and t.warning_count == 0)
    t.call("func_two_each 22 23 opt1=24 opt2=25")
    assert (t.var == (22, 23, 24, 25) and t.warning_count == 0)


class TestClass(Caller):
    def __init__(self):
        super(TestClass, self).__init__(parse_parenthesis_as_list=False)
        self.var = -1
        self.warning_count = 0

    def send_warning(self, warning: str, *args, **kwargs):
        self.warning_count += 1
        print(self.warning_count, warning)

    def func_no_args(self):
        self.var = 0

    def func_one_mand(self, mand):
        self.var = mand

    def func_one_opt(self, opt=None):
        self.var = opt

    def func_one_each(self, mand, opt=None):
        self.var = mand, opt

    def func_two_each(self, mand1, mand2, opt1=None, opt2=None):
        self.var = mand1, mand2, opt1, opt2


def test_caller_invalid_input():
    t = TestClass()

    # Nonexistent function
    with pytest.raises(InvalidInputError):
        t.call("nonexistent_function")
    assert (t.warning_count == 1)

    # Too many/few arguments
    with pytest.raises(TypeError):
        t.call("func_no_args 1")
    assert (t.warning_count == 2)

    # Too many/few arguments
    with pytest.raises(TypeError):
        t.call("func_no_args 1")
    assert (t.warning_count == 3)
    with pytest.raises(TypeError):
        t.call("func_one_mand ")
    assert (t.warning_count == 4)
    with pytest.raises(TypeError):
        t.call("func_one_mand 1 2")
    assert (t.warning_count == 5)
    with pytest.raises(TypeError):
        t.call("func_one_opt 1 2")
    assert (t.warning_count == 6)

    # Incorrect/Nonexistent variable names
    with pytest.raises(TypeError):
        t.call("func_no_args my_arg=1")
    assert (t.warning_count == 7)
    with pytest.raises(TypeError):
        t.call("func_one_mand my_arg=1")
    assert (t.warning_count == 8)
    with pytest.raises(TypeError):
        t.call("func_one_opt my_arg=1")
    assert (t.warning_count == 9)
    with pytest.raises(TypeError):
        t.call("func_one_each 1 my_arg=1")
    assert (t.warning_count == 10)

    # Duplicate argument names
    with pytest.raises(TypeError):
        t.call("func_one_each 1 mand=1")
    assert (t.warning_count == 11)
    with pytest.raises(TypeError):
        t.call("func_one_each mand=1 mand=3")
    assert (t.warning_count == 12)
    with pytest.raises(TypeError):
        t.call("func_one_each 1 2 opt=3")
    assert (t.warning_count == 13)

    # # TODO: Handle dicts separately
    # # Unhashable types as dict keys
    # # t.call("func_one_mand {[1 2 3]:v}")
    # # assert (t.warning_count == 14)
    # # t.call("func_one_mand {[]:v}")
    # # assert (t.warning_count == 15)
    # # t.call("func_one_mand {{k:v}:v}")
    # # assert (t.warning_count == 16)
    t.warning_count = 16

    # General incorrect formatting
    with pytest.raises(InvalidInputError):
        t.call("func_one_mand [")
    assert (t.warning_count == 17)
    with pytest.raises(InvalidInputError):
        t.call("func_one_mand '")
    assert (t.warning_count == 18)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each []=1")
    assert (t.warning_count == 19)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each []=1")
    assert (t.warning_count == 20)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each [] = 1")
    assert (t.warning_count == 21)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each [] == 1")
    assert (t.warning_count == 22)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each f=")
    assert (t.warning_count == 23)
    with pytest.raises(InvalidInputError):
        t.call("func_two_each [ 1 2 3 [ 3  5 6]")
    assert (t.warning_count == 24)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each [ f = 123 ]")
    assert (t.warning_count == 25)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each [ f == 123 ]")

    assert (t.warning_count == 26)
    with pytest.raises(InvalidInputError):
        t.call("func_one_each 1)")
    assert (t.warning_count == 27)

    # TODO: Handle dicts separately
    # # Various dict errors
    # t.call("func_one_mand {k:v k:v}")
    # assert (t.warning_count == 28)
    # t.call("func_one_mand {k:}")
    # assert (t.warning_count == 29)
    # t.call("func_one_mand {:v}")
    # assert (t.warning_count == 30)
    # t.call("func_one_mand {k}")
    # assert (t.warning_count == 31)
    # t.call("func_one_mand {k}")
    # assert (t.warning_count == 32)
    # t.call("func_one_mand {k:v v}")
    # assert (t.warning_count == 33)
    #
    # assert (t.var == -1)


def test_timing():
    num_iter = 1000

    print("\n\n")
    t = TestClass()
    times = []
    for i in range(num_iter):
        start = timer()
        t.func_no_args()
        times.append(timer() - start)
    print(f"[NATIVE]: Average call time for function '{t.func_no_args}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")

    times = []
    for i in range(num_iter):
        start = timer()
        t.func_one_each(12345, opt=67890)
        times.append(timer() - start)
    print(f"[NATIVE]: Average call time for function '{t.func_one_each}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")

    times = []
    for i in range(num_iter):
        start = timer()
        t.func_two_each("this is a somewhat long string compared to many other strings",
                        2, ("anotherstring", {"k": "v"}), [1, 2, 3, 4, 5, 6, 7, 8, 9])
        times.append(timer() - start)
    print(f"[NATIVE]: Average call time for function '{t.func_two_each}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")

    times = []
    for i in range(num_iter):
        start = timer()
        t.call("func_no_args")
        times.append(timer() - start)
    print(f"[PARSER]: Average call time for function '{t.func_no_args}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")

    times = []
    for i in range(num_iter):
        start = timer()
        t.call("func_one_each  12345 opt=67890")
        times.append(timer() - start)
    print(f"[PARSER]: Average call time for function '{t.func_one_each}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")

    times = []
    for i in range(num_iter):
        start = timer()
        t.call("func_two_each 'this is a somewhat long string compared to many other strings' 2 "
               "(anotherstring {k:v}) [1 2 3 4 5 6 7 8 9]")
        times.append(timer() - start)
    print(f"[PARSER]: Average call time for function '{t.func_two_each}' is {np.mean(times)} "
          f"with a max of {np.max(times)}")
