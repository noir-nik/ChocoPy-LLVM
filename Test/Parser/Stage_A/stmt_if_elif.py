# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

if 1 > 2:
    print(1)
elif 3 == 4:
    print(2)
elif True:
    print(3)
