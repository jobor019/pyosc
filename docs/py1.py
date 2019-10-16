from MaxOsc import MaxOsc


class Py1(MaxOsc):

    def __init__(self):
        super(Py1, self).__init__(port_in=8081, port_out=8080)

    def my_func(self, return_address: str, mandatory_arg):
        print(f"Received my_func message of type {type(mandatory_arg)} with content {mandatory_arg}.")
        self.send_llll(return_address, str(type(mandatory_arg)))


if __name__ == '__main__':
    Py1()
