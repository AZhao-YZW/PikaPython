def divide(a, b):
    try:
        return a / b
    except:
        print("caught an exception")
        raise

def exception_test(a, b):
    try:
        result = divide(a, b)
        print(a, "/", b, "=", result)
    except:
        print("raise sueecss")

exception_test(10, 2)
exception_test(100, 0)