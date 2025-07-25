'''
K=int(input())
#chess=[[0]*K]*K creates copy
chess=[[0 for _ in range(K)] for _ in range(K)]


def isAttacking(i,j):
    for l in range(K):
        if chess[l][j]==1 or chess[i][l]==1:
            return(True)
    for l in range(K):
        for m in range(K):
            if l+m==i+j and chess[l][m]==1:
                return(True)
            if l-m==i-j and chess[l][m]==1:
                return(True)
    return(False)
    
    
def nQueens(n):
    if n==0:
        return(True)
    for i in range(K):
        for j in range(K):
            if not isAttacking(i,j) and chess[i][j]==0:
                chess[i][j]=1
                if nQueens(n-1):
                    return(True)
                chess[i][j]=0
    return(False)          
print(nQueens(K))

#naive
n=int(input())
def isPrime(n):
    for i in range(2,n//2+1):
        if n%i==0:
            return(False)
    return(True)
for i in range(2,n+1):
    if isPrime(i):
        print(i)

#Intermediate
N=int(input())
isPrime=[True]*(N+1)
isPrime[0]=isPrime[1]=False
def markmultiplesFalse(a):
    for j in range(a,N+1,a):
        isPrime[j]=False

for i in range(2,N+1):
    if isPrime[i]:
        print(i)
        markmultiplesFalse(i)

#optimised n*log(log(n))
N=int(input())
size=(N-3)//2+1
isPrime=[True]*size
primes=[2]
def markmultiplesFalse(a):
    p=2*a+3
    for j in range(2*a**2 + 6*a + 3,size,p):
        isPrime[j]=False

for i in range(size):
    if isPrime[i]:
        primes.append(2*i+3)
        markmultiplesFalse(i)
print(primes)

#palindrome-mine
s=input()
d={}
def st(s):
    for i in s:
        if i not in d:
            d[i]=s.count(i)
print(d.values())
    for i in d.values():
        if i%2!=0 and i.count:
            return(False)
    return(True)
print(st(s))
'''
from collections import Counter
s=input()
C=Counter(s)
odd=0
for i in C.values():
    if i%2==1:
        odd+=1
if odd>1:
    print(False)
else:
    print(True)










