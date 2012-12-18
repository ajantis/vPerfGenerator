import os
import re
import shelve

import fcntl

class PlatException(Exception):
    pass

class PlatFunction:
    name = ''
    ret_type = ''
    
    # Source where function is defined
    defin = ''
    
    def __init__(self, name, ret_type, defin):
        self.name = name
        self.ret_type = ret_type
        
        self.defin = defin
        self.impl = None
        
        args = []
    
    def may_impl(self, node):
        '''Returns True if function may be implemented by node'''
        return self.impl is None or self.impl == str(node)
    
    def set_impl(self, node):
        '''Marks function to be implemented by node'''
        if not self.may_impl(node):
            return False
        
        self.impl = str(node)
        return True
    
    re = re.compile('PLATAPI\s+(.*?)\s+([A-Za-z_0-9]+)\s*\((.*?)\)')

class PlatDeclaration:
    re = re.compile('PLATAPIDECL\s*\((.*?)\)\s+(.*?);')

class PlatCacheDecorator:
    def __init__(self, mode):
        self.mode = mode
     
    def __call__(self, method):   
        def closure(cls, *args):
            cache = cls(os.environ['PLATCACHE'], self.mode)
            try:
                cache.open()
                return method(cache, *args)
            finally:
                if cache:
                    cache.close()
            
        return closure

class PlatCache:
    '''Provides concurrent access to platform cache file
    
    Implementation from http://stackoverflow.com/questions/486490/python-shelve-module-question'''
    READ = 0
    READWRITE = 1
    
    def __init__(self, file_name, mode):
        self.mode = mode
        
        if mode == PlatCache.READWRITE:
            self.lock_file_mode = 'a'
            self.lock_mode = fcntl.LOCK_EX
            self.shelve_mode = 'c'
        else:
            self.lock_file_mode = 'r'
            self.lock_mode = fcntl.LOCK_SH
            self.shelve_mode = 'r'
        
        self.file_name = file_name
        
        self.shelve = None
    
    def open(self):
        '''Open lock file than shelve'''
        self.lock_file = file(self.file_name + '.lck', self.lock_file_mode)
        fcntl.flock(self.lock_file.fileno(), self.lock_mode | fcntl.LOCK_NB)
        
        self.shelve = shelve.open(self.file_name, flag = self.shelve_mode)
    
    def close(self):
        if self.shelve:
            self.shelve.close()
            
            fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_UN)
            self.lock_file.close()
    
    @classmethod
    @PlatCacheDecorator(READWRITE)
    def delete(self, defin):
        func_names = []
        
        for func in self.shelve.values():
            func_names.append(func.name)
            
        for func_name in func_names:
            del self.shelve[func]
    
    @classmethod
    @PlatCacheDecorator(READWRITE)
    def update_func(self, func):
        self.shelve[func.name] = func

    @classmethod
    @PlatCacheDecorator(READWRITE)
    def update(self, plat_funcs):
        for func_name in plat_funcs:
            self.shelve[func_name] = plat_funcs[func_name]
    
    @classmethod
    @PlatCacheDecorator(READ)
    def read(self):
        return dict(self.shelve)