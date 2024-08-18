import sys
import time
from pathlib import Path
from run_test_common import traverse_and_test, run_repl_test

def prepare(test_dir: str):
    return True

def run_test(test_file, result_file):
    path = Path(sys.argv[1])
    run_repl_test(str(path), test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== *REPL TEST* %Y-%m-%d %H:%M:%S ===========',
                        time.localtime(time.time())))
    traverse_and_test(dir='test_case', result_fname='result_repl_mode.txt',
                      prepare=prepare, run_test=run_test)
