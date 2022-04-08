import numpy as np


while True:
    A = np.random.rand(10000,1000)
    B = np.random.rand(1000,10000)
    C = A @ B 
    dt = np.linalg.det(C)
    print(f"{dt:}")



