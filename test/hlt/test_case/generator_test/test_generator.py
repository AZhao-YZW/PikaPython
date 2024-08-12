# generator functions test
print('===== generator functions test =====')
def num_gen_func():
    for i in range(5):
        yield i

func_1 = num_gen_func()
func_2 = num_gen_func()
print('func_1: ' + str(next(func_1)))
print('func_2: ' + str(next(func_2)))
print('func_1: ' + str(next(func_1)))
print('func_2: ' + str(next(func_2)))
print('func_1: ' + str(next(func_1)))
print('func_2: ' + str(next(func_2)))
print('func_1: ' + str(next(func_1)))
print('func_2: ' + str(next(func_2)))
print('func_1: ' + str(next(func_1)))
print('func_2: ' + str(next(func_2)))

# generator expressions test
print('===== generator expressions test =====')
num_gen_express = (i for i in range(5))
print('num_gen_express: ' + str(next(num_gen_express)))
print('num_gen_express: ' + str(next(num_gen_express)))
print('num_gen_express: ' + str(next(num_gen_express)))
print('num_gen_express: ' + str(next(num_gen_express)))
print('num_gen_express: ' + str(next(num_gen_express)))
