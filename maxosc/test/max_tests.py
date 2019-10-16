from maxosc.MaxOsc import MaxOsc


class MaxTests(MaxOsc):

    def __init__(self):
        super(MaxTests, self).__init__()
        self.run()

    def no_args(self):
        print("no args")
        self.send_llll("/no_args", "bang")

    def one_mand(self, mand):
        print("one mand", mand)
        self.send_llll("/one_mand", mand)

    def one_opt(self, opt=None):
        print("one opt", opt)
        self.send_llll("/one_opt", opt)

    def one_each(self, mand, opt=None):
        print("one each", mand, opt)
        self.send_llll("/one_each", mand, opt)

    def two_each(self, mand1, mand2, opt1=None, opt2=None):
        print("two each", mand1, mand2, opt1, opt2)
        self.send_llll("/two_each", mand1, mand2, opt1, opt2)

    def throughput_raw(self, ret_adr, *args):
        print("throughput: Args are {}".format(args))
        self._client.send_message(ret_adr, args)

    def throughput_llll(self, ret_adr, *args):
        print("throughput_llll Args are {}".format(args))
        self.send_llll(ret_adr, *args)

    def throughput_dict(self, ret_adr, dd: str):
        dd_dict = self.parse_maxdict(dd)
        self.send_maxdict(ret_adr, dd_dict)


if __name__ == '__main__':
    MaxTests()
