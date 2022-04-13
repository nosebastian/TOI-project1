import numpy as np

"""
This scrip is used as artificial load for raspberry pi, in order to test the 
temperature measurement.
"""
while True:
    A = np.random.rand(10000,1000)
    B = np.random.rand(1000,10000)
    C = A @ B 
    dt = np.linalg.det(C)
    print(f"{dt:}")



