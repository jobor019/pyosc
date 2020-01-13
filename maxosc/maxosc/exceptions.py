class InvalidInputError(Exception):
    def __init__(self, message):
        super().__init__(message)


class DuplicateKeyError(Exception):
    def __init__(self, message):
        super().__init__(message)


class MaxOscError(Exception):
    def __init__(self, message):
        super().__init__(message)
