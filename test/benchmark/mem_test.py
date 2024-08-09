import PikaStdData
import PikaStdLib

print('===== mem_test.py start =====')
print('')

max_mem = 0
now_mem = 0
last_now_mem = 0
add_mem = 0

def print_mem(end: str):
    global max_mem
    global now_mem
    global last_now_mem
    global add_mem
    max_mem = PikaStdLib.MemChecker.getMax()
    last_now_mem = now_mem
    now_mem = PikaStdLib.MemChecker.getNow()
    add_mem = now_mem - last_now_mem
    info = '%.2f kB \t%.2f kB \t%.2f kB \t%s' % (max_mem, now_mem, add_mem, end)
    print(info)

print('max_mem \tnow_mem \tadd_mem \t discription\t')
print('------- \t------- \t------- \t -----------\t')
print_mem(' - start')
print('')

num_list = []
print_mem(' - number list create')
for i in range(100):
    num_list.append(i)
print_mem(' - number list add')
for i in range(100):
    num_list.remove(i)
del num_list
print_mem(' - number list delete')
print('')

str_list = []
print_mem(' - string list create')
for i in range(100):
    str_list.append('%d' % i)
print_mem(' - string list add')
for i in range(100):
    str_list.remove('%d' % i)
del str_list
print_mem(' - string list delete')
print('')

list_list = []
print_mem(' - list list create')
for i in range(100):
    list_list.append([i])
print_mem(' - list list add')
for i in range(100):
    list_list.remove([i])
del list_list
print_mem(' - list list delete')
print('')

tuple_list = []
print_mem(' - tuple list create')
for i in range(100):
    tuple_list.append((i, '%d' % i))
print_mem(' - tuple list add')
for i in range(100):
    tuple_list.remove((i, '%d' % i))
del tuple_list
print_mem(' - tuple list delete')
print('')

directory_list = []
print_mem(' - directory list create')
for i in range(100):
    directory_list.append({'key_%d' % i: i})
print_mem(' - directory list add')
for i in range(100):
    directory_list.remove({'key_%d' % i: i})
del directory_list
print_mem(' - directory list delete')
print('')

print('===== mem_test.py end =====')