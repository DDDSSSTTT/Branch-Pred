import sys
import os

files = os.listdir("./unzipped/")
for file in files:
    unique_addr = set()
    with open("./unzipped/" + file) as fp:
        line=fp.readline()
        while line:
            addr = line.split(" ")[0]
            unique_addr.add(addr)
            line = fp.readline()
        fp.close()
        unique_addr = list(unique_addr)
        unique_addr.sort()
        # print(unique_addr)
        print(file, len(unique_addr))
