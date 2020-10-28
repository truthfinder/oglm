A = 7
Q = 4
M = Q - 1
print(A / Q)
print(A & (M-1))
print(A % Q) 

def qbase(idx, sz):
	return idx & ~M

def qcount(idx, sz):
	return (idx + sz + Q) / Q

print("3 % Q = 3", ((3 % Q == 3)))

print("qbase (3, 2) = 0", qbase(3,2), qbase(3,2)==0)
print("qcount(3, 2) = 2", qcount(3,2), qcount(3,2)==2)

