import os
import time
import subprocess
from pathlib import Path

def run_test():
    if not os.path.exists('result.txt'):
        os.system('touch result.txt')
    with open('./result.txt', 'w', encoding='utf-8') as f:
        f.truncate(0)
    with open('./testcase.py', 'r', encoding='utf-8') as fin, \
         open('./result.txt', 'a', encoding='utf-8') as fout:
        path = Path('./../../build/output/leafpython')
        p = subprocess.Popen(str(path),
                            stdin=fin,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            shell=True)
        out, err = p.communicate()
        out_disp = out.decode().replace('\r\n', '\n').replace('\r', '\n').replace('\n\n', '\n')
        print(out_disp)
        print(out_disp, file=fout)

if __name__ == '__main__':
    print(time.strftime('=========== %Y-%m-%d %H:%M:%S ===========',time.localtime(time.time())))
    run_test()
