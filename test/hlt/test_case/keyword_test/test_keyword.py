print('===== start keyword test =====')

# "while " 
while 0 == 1:
    print('while test: unable to reach')

while 1 == 1:
    print('while test: able to reach')
    break

while 1 == 1 :
    print('while test: able to reach')
    break

while 1==1:
    print('while test: able to reach')
    break

# "if "
if 0 == 1:
    print('if test: unable to reach')

if 1 == 1:
    print('if test: able to reach')

if 1 == 1 :
    print('if test: able to reach')

if 1==1:
    print('if test: able to reach')

# "elif "
if 0 == 1:
    print('elif test: unable to reach')
elif 1 == 1:
    print('elif test: able to reach')

if 0 == 1:
    print('elif test: unable to reach')
elif 1 == 1 :
    print('elif test: able to reach')

if 0 == 1:
    print('elif test: unable to reach')
elif 1==1:
    print('elif test: able to reach')

# "else"
if 0 == 1:
    print('else test: unable to reach')
else:
    print('else test: able to reach')

if 0 == 1:
    print('else test: unable to reach')
else :
    print('else test: able to reach')

if 1 == 1:
    print('else test: able to reach')
else:
    print('else test: unable to reach')

# "break"
while 1 == 1:
    print('break test: only execute once')
    break  # spaces

while 1 == 1:
    print('break test: only execute once')
    break

# "continue"
for i in range(5):
    if i < 3:
        continue  # spaces
    print('continue test: execute twice')

for i in range(5):
    if i < 3:
        continue
    print('continue test: execute twice')

# "return"
def return_test_func1():
    print('return test: able to reach')
    return  # spaces
    print('return test: unable to reach')

return_test_func1()

def return_test_func2():
    print('return test: able to reach')
    return
    print('return test: unable to reach')

return_test_func2()

# "for "
for i in range(2):
    print('for test: execute twice')

for i in range(2) :
    print('for test: execute twice')

# "global "
def global_test_func():
    global foo
    global  bar  # spaces
    tmp = 1
    foo = 2
    bar = 3
    print(tmp)
    print(foo)
    print(bar)

global_test_func()

try:
    print(tmp)
    print('global test: tmp is able to reach')
except:
    print('global test: tmp is unable to reach')

try:
    print(foo)
    print('global test: foo is able to reach')
except:
    print('global test: foo is unable to reach')

try:
    print(bar)
    print('global test: bar is able to reach')
except:
    print('global test: bar is unable to reach')

# "del "
del_test_var1 = 1
del_test_var2 = 2
del del_test_var1
del  del_test_var2  # spaces

try:
    print(del_test_var1)
    print('del test: del_test_var1 is not delete')
except:
    print('del test: del_test_var1 is delete')

try:
    print(del_test_var2)
    print('del test: del_test_var2 is not delete')
except:
    print('del test: del_test_var2 is delete')

# "del("
del_test_var3 = 3
del_test_var4 = 4
del(del_test_var3)
del  (del_test_var4)  # spaces

try:
    print(del_test_var3)
    print('del test: del_test_var3 is not delete')
except:
    print('del test: del_test_var3 is delete')

try:
    print(del_test_var4)
    print('del test: del_test_var4 is not delete')
except:
    print('del test: del_test_var4 is delete')

# "def "
try:
    def def_test_func1():
        print('def_test_func1')
    
    print('def test: def_test_func1 defined ok')
    def_test_func1()
except:
    print('def test: def_test_func1 defined failed')

try:
    def def_test_func2  (  )  :  # spaces
        print('def_test_func2')
    
    print('def test: def_test_func2 defined ok')
    def_test_func2()
except:
    print('def test: def_test_func2 defined failed')

# below codes will failed while compiling:
# try:
#     def def_test_func3:
#         print('def_test_func1')

#     print('def test: def_test_func3 defined ok')
#     def_test_func3()
# except:
#     print('def test: def_test_func3 defined failed')

# "class "
try:
    class class_test_func1():
        def __init__(self) -> None:
            print('class_test_func1')
    
    print('class test: class_test_func1 defined ok')
    class_test_func1()
except:
    print('class test: class_test_func1 defined failed')

try:
    class class_test_func2  (  )  :  # spaces
        def __init__(self) -> None:
            print('class_test_func2')
    
    print('class test: class_test_func2 defined ok')
    class_test_func2()
except:
    print('class test: class_test_func2 defined failed')

try:
    class class_test_func3:
        def __init__(self) -> None:
            print('class_test_func3')
    
    print('class test: class_test_func3 defined ok')
    class_test_func3()
except:
    print('class test: class_test_func3 defined failed')

try:
    class   class_test_func4  :  # spaces
        def __init__(self) -> None:
            print('class_test_func4')
    
    print('class test: class_test_func4 defined ok')
    class_test_func4()
except:
    print('class test: class_test_func4 defined failed')

# "try" "except"
try:
    print('try except test: able to reach')
except:
    print('try except test: unable to reach')

try:
    raise
    print('try except test: unable to reach')
except:
    print('try except test: able to reach')

try  :  # spaces
    print('try except test: able to reach')
except:
    print('try except test: unable to reach')

try:
    raise
    print('try except test: unable to reach')
except  :  # spaces
    print('try except test: able to reach')

# "raise"
try:
    raise
    print('raise test: unable to reach')
except:
    print('raise test: able to reach')

try:
    raise RuntimeError("Raise Test Error")      # unsupport now
    print('raise test: unable to reach')
except:
    print('raise test: able to reach')

# "assert "
try:
    assert 1 == 1, 'Assert Test Error'
    print('assert test: able to reach')
except:
    print('assert test: unable to reach')

try:
    assert 1 == 0, 'Assert Test Error'
    print('assert test: unable to reach')
except:
    print('assert test: able to reach')