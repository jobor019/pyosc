from timeit import default_timer as timer

import numpy as np
import pytest
from maxosc.caller import Caller
from maxosc.exceptions import MaxOscError


class TestClass(Caller):
    def __init__(self, parse_parenthesis_as_list: bool, discard_duplicate_args: bool):
        super(TestClass, self).__init__(parse_parenthesis_as_list, discard_duplicate_args)
        self.var = -1

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


def test_caller_default_valid_input():
    t = TestClass(parse_parenthesis_as_list=False, discard_duplicate_args=False)

    t.call("func_no_args")
    assert t.var == 0
    t.call("func_one_mand 1")
    assert t.var == 1
    t.call("func_one_mand mand=True")
    assert t.var == True
    t.call("func_one_mand mand=     False")
    assert t.var == False
    t.call("func_one_mand []")
    assert t.var == []
    t.call("func_one_mand [1 2 3]")

    t.call("func_one_opt")
    assert t.var == None
    t.call("func_one_opt 4")
    assert t.var == 4
    t.call("func_one_opt opt=5")
    assert t.var == 5

    t.call("func_one_each 6")
    assert t.var == (6, None)
    t.call("func_one_each 7 8")
    assert t.var == (7, 8)
    t.call("func_one_each 9 opt=10")
    assert t.var == (9, 10)
    t.call("func_one_each mand=11 opt=12")
    assert t.var == (11, 12)
    t.call("func_one_each opt=-1 mand=-2")
    assert t.var == (-2, -1)
    t.call("func_one_each opt=-2 -3")
    assert t.var == (-3, -2)

    t.call("func_two_each 13 14")
    assert t.var == (13, 14, None, None)
    t.call("func_two_each 15 16 17")
    assert t.var == (15, 16, 17, None)
    t.call("func_two_each 18 19 20 21")
    assert t.var == (18, 19, 20, 21)
    t.call("func_two_each 22 23 opt1=24 opt2=25")
    assert t.var == (22, 23, 24, 25)


def test_caller_parse_parenthesis():
    t = TestClass(True, False)

    # Valid input:
    t.call("func_one_mand (1 2 3 4 5)")
    assert t.var == [1, 2, 3, 4, 5]
    t.call("func_one_mand ( )")
    assert t.var == []
    t.call("func_one_mand ( 1 )")
    assert t.var == [1]


def test_caller_discard_duplicates():
    t = TestClass(False, True)

    # Valid input:
    t.call("func_two_each 1 2 mand1=3 opt1=4 opt2=5")
    assert t.var == (1, 2, 4, 5)
    t.call("func_two_each 6 7 8 9 mand1=10 mand2=11 opt1=12 opt2=13")
    assert t.var == (6, 7, 8, 9)
    t.call("func_two_each mand1=14 mand2=15 opt1=16 opt2=17 mand1=18, mand1=19")
    assert t.var == (14, 15, 16, 17)

    # Invalid input
    with pytest.raises(TypeError):
        t.call("func_two_each mand1=1 2 3 4")


def test_caller_default_invalid_input():
    t = TestClass(False, False)

    # Nonexistent function
    with pytest.raises(MaxOscError):
        t.call("nonexistent_function")

    # Too many/few arguments
    with pytest.raises(TypeError):
        t.call("func_no_args 1")

    # Too many/few arguments
    with pytest.raises(TypeError):
        t.call("func_no_args 1")
    with pytest.raises(TypeError):
        t.call("func_one_mand ")
    with pytest.raises(TypeError):
        t.call("func_one_mand 1 2")
    with pytest.raises(TypeError):
        t.call("func_one_opt 1 2")

    # Incorrect/Nonexistent variable names
    with pytest.raises(TypeError):
        t.call("func_no_args my_arg=1")
    with pytest.raises(TypeError):
        t.call("func_one_mand my_arg=1")
    with pytest.raises(TypeError):
        t.call("func_one_opt my_arg=1")
    with pytest.raises(TypeError):
        t.call("func_one_each 1 my_arg=1")

    # Duplicate argument names
    with pytest.raises(TypeError):
        t.call("func_one_each 1 mand=1")
    with pytest.raises(MaxOscError):
        t.call("func_one_each mand=1 mand=3")
    with pytest.raises(TypeError):
        t.call("func_one_each 1 2 opt=3")

    # General incorrect formatting
    with pytest.raises(MaxOscError):
        t.call("func_one_mand [")
    with pytest.raises(MaxOscError):
        t.call("func_one_mand '")
    with pytest.raises(MaxOscError):
        t.call("func_one_each []=1")
    with pytest.raises(MaxOscError):
        t.call("func_one_each []=1")
    with pytest.raises(MaxOscError):
        t.call("func_one_each [] = 1")
    with pytest.raises(MaxOscError):
        t.call("func_one_each [] == 1")
    with pytest.raises(MaxOscError):
        t.call("func_one_each f=")
    with pytest.raises(MaxOscError):
        t.call("func_two_each [ 1 2 3 [ 3  5 6]")
    with pytest.raises(MaxOscError):
        t.call("func_one_each [ f = 123 ]")
    with pytest.raises(MaxOscError):
        t.call("func_one_each [ f == 123 ]")
    with pytest.raises(MaxOscError):
        t.call("func_one_each 1)")


def test_timing():
    num_iter = 1000

    print("\n\n")
    t = TestClass(False, False)
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

def test_timing_discard_duplicates():
    num_iter = 1000

    print("\n\n")
    t = TestClass(False, True)
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