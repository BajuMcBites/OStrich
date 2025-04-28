input = '52 55 a 0 2 3 40 54 0 12 34 57 8 0 45 0 0 38 0 69 0 0 40 11 62 3b a 0 2 f a 0 2 3 0 16 0 35 0 24 9a f4 5 10 2 0 0 1 0 0 0 0 0 0 6 67 6f 6f 67 6c 65 3 63 6f 6d 0 0 1 0 1'


for part in input.split(): 
    if len(part) == 2:
        print(part, end=' ')
    else: 
        print(f'0{part}', end=' ')
    
print()