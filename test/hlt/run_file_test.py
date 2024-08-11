import os
import sys
import time
from pathlib import Path
from run_test_common import traverse_and_test, run_file_test

leafpython = Path(sys.argv[1])
pika_dir = Path(sys.argv[2])

def run_test(test_file: str, result_file: str):
    os.system(f'cp {test_file} {pika_dir}/main.py')
    run_file_test(f'./{pika_dir}/leafpython {pika_dir}/main.py', test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== *FILE TEST* %Y-%m-%d %H:%M:%S ===========',
                        time.localtime(time.time())))
    os.system(f'cp {leafpython} {pika_dir}')
    os.system(f'mv {pika_dir}/main.py {pika_dir}/main.py.bak')
    traverse_and_test(dir='test_case', result_fname='result_file_mode.txt', run_test=run_test)
    os.system(f'rm -f {pika_dir}/leafpython')
    os.system(f'rm -f {pika_dir}/main.py')
    os.system(f'mv {pika_dir}/main.py.bak {pika_dir}/main.py')
