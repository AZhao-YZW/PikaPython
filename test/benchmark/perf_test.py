import time

def perf_test(code_to_test, iterations=10):
    start_time = time.time()
    for _ in range(iterations):
        code_to_test()
    end_time = time.time()
    
    total_time = end_time - start_time
    average_time = total_time / iterations
    
    print('total time:', total_time, 's')
    print('average time:', average_time, 's')

def test_code():
    num_list = []
    for i in range(1000):
        num_list.append(i)
    for i in range(1000):
        num_list.remove(i)
    del num_list

perf_test(test_code, 5)


