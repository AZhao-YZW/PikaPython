import time
from run_test_common import traverse_and_test, run_file_test

def prepare(test_dir: str):
    return True

def run_test(test_file, result_file):
    run_file_test(f'python {test_file}', test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== *CPYTHON FILE TEST* %Y-%m-%d %H:%M:%S ===========',
                        time.localtime(time.time())))
    traverse_and_test(dir='test_case', result_fname='result_cpy_file_mode.txt',
                      prepare=prepare, run_test=run_test)
