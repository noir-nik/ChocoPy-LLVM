# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:str = "Hello"
y:str = "World"
z:str = ""

1 + x
z = x + 1
z = x[0] = y
x[1] = y
x[True]

