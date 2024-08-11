import time
from run_test_common import traverse_and_test, run_repl_test

def run_test(test_file, result_file):
    run_repl_test('python', test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== *CPYTHON REPL TEST* %Y-%m-%d %H:%M:%S ===========',
                        time.localtime(time.time())))
    traverse_and_test(dir='test_case', result_fname='result_cpy_repl_mode.txt', run_test=run_test)
