import os
import sys
import time
import subprocess
from pathlib import Path

def run_test(test_file, result_file):
   with open(test_file, 'r', encoding='utf-8') as fin, \
         open(result_file, 'a', encoding='utf-8') as fout:
        path = Path(sys.argv[1])
        p = subprocess.Popen(str(path),
                            stdin=fin,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            shell=True)
        out, err = p.communicate()
        out_disp = out.decode(errors='ignore').replace('\r\n', '\n').replace('\r', '\n').replace('\n\n', '\n')
        print(f"Results for {test_file}:")
        print(out_disp)
        fout.write(f"Results for {test_file}:\n")
        fout.write(out_disp + '\n')

def traverse_and_test(directory):
    for root, _, files in os.walk(directory):
        if root != directory:
            result_file = os.path.join(root, 'result.txt')
            with open(result_file, 'w', encoding='utf-8') as f:
                f.truncate(0)
            for file in files:
                if file.endswith('.py'):
                    test_file = os.path.join(root, file)
                    run_test(test_file, result_file)

if __name__ == '__main__':
    print(time.strftime('=========== %Y-%m-%d %H:%M:%S ===========',time.localtime(time.time())))
    traverse_and_test('test_case')
