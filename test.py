from BristolMatchingEngine import *
from time import time

tvec = TraderVector()

for i in range(100_000):
    tvec.append(Trader(True, False))
    
for i in range(100_000):
    tvec.append(Trader(False, True))

x = LimitOrderBook()

t0 = time()

x.run_experiment(0, 1000, tvec)

print(time() - t0)

print(len(x.get_executed_transactions()))
