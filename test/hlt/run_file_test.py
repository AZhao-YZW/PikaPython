import os
import sys
import time
import subprocess
from pathlib import Path

leafpython = Path(sys.argv[1])
pika_dir = Path(sys.argv[2])

def run_test(test_file, result_file):
    os.system(f'cp {test_file} {pika_dir}/main.py')
    print(test_file)
    with open(result_file, 'a', encoding='utf-8') as fout:
        p = subprocess.Popen(f'./{pika_dir}/leafpython {pika_dir}/main.py',
                            stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            shell=True)
        out, err = p.communicate()
        out_disp = out.decode(errors='ignore').replace('\r\n', '\n').replace('\r', '\n').replace('\n\n', '\n')
        print(f"Results for {test_file}:")
        print(out_disp)
        fout.write(f"Results for {test_file}:\n")
        fout.write(out_disp)

def traverse_and_test(directory):
    for root, _, files in os.walk(directory):
        if root != directory:
            result_file = os.path.join(root, 'result_file_mode.txt')
            with open(result_file, 'w', encoding='utf-8') as f:
                f.truncate(0)
            for file in files:
                if file.endswith('.py'):
                    test_file = os.path.join(root, file)
                    run_test(test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== %Y-%m-%d %H:%M:%S ===========',time.localtime(time.time())))
    os.system(f'cp {leafpython} {pika_dir}')
    os.system(f'mv {pika_dir}/main.py {pika_dir}/main.py.bak')
    traverse_and_test('test_case')
    os.system(f'rm -f {pika_dir}/leafpython')
    os.system(f'rm -f {pika_dir}/main.py')
    os.system(f'mv {pika_dir}/main.py.bak {pika_dir}/main.py')
