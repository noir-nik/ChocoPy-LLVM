# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

x:int = None

x = [1, 2, 3]
x[0] = x[1]
