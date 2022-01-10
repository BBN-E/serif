"""
Backwards compatibility with previous versions of Python (2.4 or
earlier).

This module provides backwards compatibility by defining
functions and classes that were not available in earlier versions of
Python.
"""

# Drop-in replacement for 'any' in versions of Python where it is not
# yet included.
try:
    any
except NameError:
    print 'Python 2.4 compatibility mode'
    def any(iterable):
        for v in iterable:
            if v: return True
        return False
    __builtins__['any'] = any

# Drop-in replacement for 'collections.defaultdict' in versions of
# Python where it is not yet included.
#
# Written by Jason Kirtland from Python cookbook.
# Licensed under the PSF License.
# <http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/523034>
try:
    from collections import defaultdict
except ImportError:
    class defaultdict(dict):
        def __init__(self, default_factory=None, *a, **kw):
            if (default_factory is not None and
                not hasattr(default_factory, '__call__')):
                raise TypeError('first argument must be callable')
            dict.__init__(self, *a, **kw)
            self.default_factory = default_factory
        def __getitem__(self, key):
            try:
                return dict.__getitem__(self, key)
            except KeyError:
                return self.__missing__(key)
        def __missing__(self, key):
            if self.default_factory is None:
                raise KeyError(key)
            self[key] = value = self.default_factory()
            return value
        def __reduce__(self):
            if self.default_factory is None:
                args = tuple()
            else:
                args = self.default_factory,
            return type(self), args, None, None, self.iteritems()
        def copy(self):
            return self.__copy__()
        def __copy__(self):
            return type(self)(self.default_factory, self)
        def __deepcopy__(self, memo):
            import copy
            return type(self)(self.default_factory,
                              copy.deepcopy(self.items()))
        def __repr__(self):
            return 'defaultdict(%s, %s)' % (self.default_factory,
                                            dict.__repr__(self))
    import collections
    collections.defaultdict = defaultdict
