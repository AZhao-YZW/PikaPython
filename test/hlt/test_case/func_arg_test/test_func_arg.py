def test_function_arguments(a, b=2, *args, **kwargs):
    print("a:", a)
    print("b:", b)
    print("*args:", args)
    print("**kwargs:", kwargs)

print('======= Test Start =======')
print("\nTest function: test_function_arguments(a, b=2, *args, **kwargs)")
print("\nFunction arguments test:")
test_function_arguments(1)
test_function_arguments(1, 3)
test_function_arguments(1, 3, 4, 5, 6)
test_function_arguments(1, 3, 4, 5, 6, x=7, y=8)
print("\nFunction arguments test with keyword arguments:")
# below codes will failed while compiling:
# test_function_arguments(a=1)
# test_function_arguments(a=1, b=3)
# test_function_arguments(a=1, b=3, x=7, y=8)
print('======= Test End =======')