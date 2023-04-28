from random import *
def randstr():
    M = "abcdefghijklmnopqrstuvwxyz"
    Len = 5
    ret = ""
    for i in range(Len):
        ret += choice(M)
    return ret

n = 500000
pI, pD, pF = 1, 1, 1
dic = []

f = open("t.in", "w", encoding="utf-8")
print(n, file=f)
for i in range(n):
    opt = randint(1, pI + pD + pF)
    if (opt <= pI or len(dic) == 0):
        key = randstr()
        val = randint(0, 100)
        print("insert", key, val, file=f)
        dic.append((key, val))
    elif (opt <= pI + pD):
        key, val = choice(dic)
        print("delete", key, val, file=f)
    elif (opt <= pI + pD+pF):
        if (random() < 0.7):
            key, val = choice(dic)
            print("find", key, file=f)
        else:
            print("find", randstr(), file=f)
