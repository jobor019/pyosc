from maxosc.maxosc import MaxOsc


class MaxTestDefault(MaxOsc):

    def __init__(self):
        super(MaxTestDefault, self).__init__()
        self.run()

    def no_args(self):
        print("no args")
        return "bang"

    def no_return(self):
        print("no return")

    def one_mand(self, mand):
        print("one mand", mand)
        return mand

    def one_opt(self, opt=None):
        print("one opt", opt)
        return opt

    def one_each(self, mand, opt=None):
        print("one each", mand, opt)
        return mand, opt

    def two_each(self, mand1, mand2, opt1=None, opt2=None):
        print("two each", mand1, mand2, opt1, opt2)
        return mand1, mand2, opt1, opt2

    @staticmethod
    def stat_noarg():
        return "bang"

    @staticmethod
    def stat_arg(a1):
        return a1


if __name__ == '__main__':
    MaxTestDefault()
