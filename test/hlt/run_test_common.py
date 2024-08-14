import os
import subprocess
from typing import Callable

def run_test_and_output(p: subprocess.Popen, ftest: str, fout):
    out, err = p.communicate()
    out_disp = out.decode(errors='ignore').replace('\r\n', '\n').replace('\r', '\n').replace('\n\n', '\n')
    print(f"{ftest} OK")
    fout.write(f"Result for {ftest}:\n")
    fout.write(out_disp)

def run_repl_test(py_cmd: str, test_file: str, result_file: str):
    with open(test_file, 'r', encoding='utf-8') as fin, \
         open(result_file, 'a', encoding='utf-8') as fout:
        p = subprocess.Popen(py_cmd, stdin=fin, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, shell=True)
        run_test_and_output(p, test_file, fout)

def run_file_test(py_cmd: str, test_file: str, result_file: str):
    with open(result_file, 'a', encoding='utf-8') as fout:
        p = subprocess.Popen(py_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, shell=True)
        run_test_and_output(p, test_file, fout)

def traverse_and_test(dir: str, result_fname: str, run_test: Callable[[str, str], None]):
    for root, _, files in os.walk(dir):
        if root != dir:
            result_file = os.path.join(root, result_fname)
            with open(result_file, 'w', encoding='utf-8') as f:
                f.truncate(0)
            for file in files:
                if file.endswith('.py'):
                    test_file = os.path.join(root, file)
                    run_test(test_file, result_file)