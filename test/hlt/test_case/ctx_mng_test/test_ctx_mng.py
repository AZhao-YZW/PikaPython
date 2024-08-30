# with statement

# custom context managers (using enter and exit methods)
print('===== custom context managers test =====')

class InnerContext:
    def __init__(self):
        print('InnerContext.__init__')
    
    def do_something(self):
        print('InnerContext.do_something')
    
    def __del__(self):
        print('InnerContext.__del__')
        print(' ')

class ContextManager:
    def __init__(self, flag):
        print('ContextManager.__init__(%s)' % flag)
        self.flag = flag
    
    def __enter__(self):
        print('ContextManager.__enter__')
        return InnerContext()
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        print('ContextManager.__exit__(%s, %s)' % (exc_type, exc_val))
        return self.flag

# below codes will failed while compiling:
# with ContextManager(True) as ctx:
#     ctx.do_something()
#     raise RuntimeError('error message handled')

# with ContextManager(False) as ctx:
#     ctx.do_something()
#     raise RuntimeError('error message propagated')