import hashlib
import base64
import sys


def main(): 
    dict = sys.argv[1]
    input = sys.argv[2]
    
    
    hash = {}
    found = 0 
    f = open(input,"r")
    for line in f: 
        line = line.rstrip() 
        hash[line] = line 
    f.close() 
    
    file = open(dict,"r") 
    found = open("cracked.txt","w")
    for line in file:
        line = line.rstrip() 
        hash_obj = hashlib.sha256("CMSC414"+line+"Fall16")
        hex_dig = hash_obj.hexdigest() 
        result = hex_dig.decode("hex").encode("base64").rstrip()
        if result in hash.keys(): 
            hash[result] = line
    
    for k,v in hash.items():
        found.write(v + '\n')
    
    
            
if __name__ == '__main__':
    main() 
