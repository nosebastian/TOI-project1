from pip import main

from datetime import datetime
import time

if __name__ == '__main__':
    while True:
        print("Hello @ " + str(datetime.now()))
        time.sleep(5)