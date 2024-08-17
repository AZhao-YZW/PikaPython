def test_if_statement(num):
    if num > 0:
        assert True, "Expected positive number"
    elif num == 0:
        assert True, "Expected zero"
    else:
        assert False, "Expected non-negative number"

def test_while_loop(start, end):
    i = start
    while i < end:
        assert i < end, "i should be less than end"
        i += 1
    assert i == end, "After loop, i should be equal to end"

def test_for_in_list(list1):
    for item in list1:
        assert isinstance(item, int), "Expected items to be integers"
        if item == 3:
            break

def test_for_in_range(start, end):
    for i in range(start, end):
        assert i < end, "i should be less than end"
    assert i + 1 == end, "After loop, i + 1 should be equal to end"

def test_for_in_dict(dict1):
    for key in dict1:
        assert key in ['a', 'b', 'c'], "Expected keys to be in 'a', 'b', 'c'"
        if key == 'b':
            continue
        assert key != 'b', "Should not reach here if key is 'b'"

def test_if_elif_else(num):
    if num > 10:
        assert True, "Expected number greater than 10"
    elif num < 10:
        assert False, "Expected number not to be less than 10"
    else:
        assert True, "Expected number to be 10"

def test_for_break_continue(list1):
    for i, item in enumerate(list1):
        if item == 3:
            break
        if item % 2 == 0:
            continue
        assert item % 2 != 0, "Expected odd numbers only when not continuing"

def test_while_break_continue(start, end):
    i = start
    while i < end:
        if i == 3:
            break
        if i % 2 == 0:
            i += 1
            continue
        assert i % 2 != 0, "Expected odd numbers only when not continuing"
        i += 1

def run_control_flow_tests():
    num = 5
    start = 0
    end = 10
    list1 = [1, 2, 3, 4, 5]
    dict1 = {'a': 1, 'b': 2, 'c': 3}

    print('======= Test Start =======')
    
    test_if_statement(num)
    test_while_loop(start, end)
    test_for_in_list(list1)
    test_for_in_range(start, end)
    test_for_in_dict(dict1)
    test_if_elif_else(10)
    test_for_break_continue(list1)
    test_while_break_continue(start, end)

    print("Control flow tests passed!")


run_control_flow_tests()